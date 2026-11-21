/*
 * Network TX RTMP - Real-Time Messaging Transport Implementation
 *
 * Sends MPEG-TS/H.264 over RTMP (TCP-based) to MediaMTX or other RTMP servers.
 *
 * RTMP Handshake:
 *   C0 (1 byte: version 3) + C1 (1536 bytes)  -> client
 *   S0 (1 byte) + S1 (1536 bytes) + S2 (1536 bytes) <- server
 *
 * Then AMF0 commands over Chunk Stream:
 *   connect    -> "connect" with app, flashVer, tcUrl, etc.
 *   createStream -> "createStream"
 *   publish    -> "publish" with stream name
 *
 * Data sent as raw H.264 NAL units (RTMP video message type 0x09).
 */

#include "network_tx_rtmp.h"
#include "h264_encoder.h"
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "string.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <lwip/sockets.h>

static const char *TAG = "NETWORK_TX_RTMP";

/* RTMP protocol constants */
#define RTMP_CHUNK_SIZE 4096
#define RTMP_CHUNK_PROTOCOL_CONTROL 2
#define RTMP_CHUNK_INVOKE 3
#define RTMP_CHUNK_SOURCE 4
#define RTMP_CHUNK_VIDEO 5

#define RTMP_CMD_CONNECT       0x01
#define RTMP_CMD_CREATE_STREAM 0x03
#define RTMP_CMD_PUBLISH       0x05

/* RTMP state */
static struct {
    int               tcp_fd;
    bool              connected;
    bool              stream_published;
    char              server[64];
    uint16_t          port;
    char              app[64];
    char              stream[128];
    char              user[64];
    char              pass[64];
    uint32_t          chunk_size;
    uint32_t          timestamp;
    uint32_t          stream_id;
    SemaphoreHandle_t mutex;
    bool              metadata_sent;
    bool              avc_config_sent;
} s_rtmp = {0};

/* Alternative stream IDs to try if MediaMTX rejects the primary one */
static const char *s_rtmp_alt_streams[] = {
    "sse_drone_f71919",
    "stanag",
    "drone",
    "live",
    "live/stanag",
};
#define S_RTMP_ALT_STREAMS_COUNT (sizeof(s_rtmp_alt_streams) / sizeof(s_rtmp_alt_streams[0]))
static int s_rtmp_alt_stream_idx = 0;

/* Try the next alternative stream ID */
static void rtmp_next_stream_id(void)
{
    s_rtmp_alt_stream_idx = (s_rtmp_alt_stream_idx + 1) % S_RTMP_ALT_STREAMS_COUNT;
    strncpy(s_rtmp.stream, s_rtmp_alt_streams[s_rtmp_alt_stream_idx],
            sizeof(s_rtmp.stream) - 1);
    ESP_LOGW(TAG, "RTMP: trying alternate stream ID: %s", s_rtmp.stream);
}

/* Read exactly len bytes from socket (handles non-blocking EAGAIN/EWOULDBLOCK) */
static int rtmp_read(int fd, uint8_t *buf, int len)
{
    int total = 0;
    int retries = 0;
    int max_retries = 500; /* ~5 seconds at 10ms intervals */
    while (total < len) {
        int r = recv(fd, buf + total, len - total, 0);
        if (r < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (++retries > max_retries) {
                    ESP_LOGE(TAG, "rtmp_read: timeout after %d retries", retries);
                    return -1;
                }
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            ESP_LOGE(TAG, "rtmp_read: recv failed: errno %d", errno);
            return -1;
        }
        if (r == 0) {
            return -1; /* connection closed */
        }
        total += r;
        retries = 0; /* reset retry counter on progress */
    }
    return total;
}

/* Check if socket has data available to read (select with timeout_ms) */
static int rtmp_select_readable(int fd, int timeout_ms)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    int r = select(fd + 1, &fds, NULL, NULL, &tv);
    if (r < 0) return -1;
    if (r == 0) return 0; /* timeout */
    return 1; /* data available */
}

/* Drain any available response data from server (non-blocking, short timeout)
 * We only read data that's already available - don't block waiting for more.
 * This prevents long delays between RTMP commands that could cause
 * MediaMTX to reset the connection (ECONNRESET). */
static void rtmp_drain_response(int fd)
{
    uint8_t dummy[512];
    int total = 0;
    while (rtmp_select_readable(fd, 50) > 0) {
        int r = recv(fd, dummy, sizeof(dummy), 0);
        if (r <= 0) break;
        total += r;
    }
    if (total > 0) {
        ESP_LOGD(TAG, "RTMP: drained %d bytes of server response", total);
    }
}

/* ------------------------------------------------------------------ */
/* Helpers                                                            */
/* ------------------------------------------------------------------ */

static int rtmp_write(int fd, const uint8_t *data, int len)
{
    int total = 0;
    while (total < len) {
        int r = send(fd, data + total, len - total, 0);
        if (r < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            ESP_LOGE(TAG, "RTMP: send() failed: errno %d", errno);
            return -1;
        }
        total += r;
    }
    return total;
}

static void rtmp_put_be16(uint8_t *p, uint16_t v)
{
    *p++ = (v >> 8) & 0xFF;
    *p++ = v & 0xFF;
}

static void rtmp_put_be24(uint8_t *p, uint32_t v)
{
    *p++ = (v >> 16) & 0xFF;
    *p++ = (v >> 8) & 0xFF;
    *p++ = v & 0xFF;
}

static void rtmp_put_be64(uint8_t *p, double v)
{
    uint64_t bits;
    memcpy(&bits, &v, 8);
    *p++ = (bits >> 56) & 0xFF;
    *p++ = (bits >> 48) & 0xFF;
    *p++ = (bits >> 40) & 0xFF;
    *p++ = (bits >> 32) & 0xFF;
    *p++ = (bits >> 24) & 0xFF;
    *p++ = (bits >> 16) & 0xFF;
    *p++ = (bits >> 8) & 0xFF;
    *p++ = bits & 0xFF;
}

static void rtmp_put_amf_string(uint8_t **p, const char *s)
{
    int len = (int)strlen(s);
    rtmp_put_be16(*p, len);
    memcpy(*p, s, len);
    *p += len;
}

static void rtmp_put_amf_number(uint8_t **p, double v)
{
    *(*p)++ = 0x00; /* AMF_NUMBER */
    rtmp_put_be64(*p, v);
    *p += 8;
}

static void rtmp_put_amf_object_prop(uint8_t **p, const char *key, double v)
{
    rtmp_put_amf_string(p, key);
    rtmp_put_amf_number(p, v);
}

static void rtmp_put_amf_number_prop(uint8_t **p, const char *key, double v)
{
    rtmp_put_amf_string(p, key);
    rtmp_put_amf_number(p, v);
}

static void rtmp_put_amf_object_end(uint8_t **p)
{
    rtmp_put_be24(*p, 0x0009); /* object end marker */
}

static void rtmp_put_amf_string_prop(uint8_t **p, const char *key, const char *v)
{
    rtmp_put_amf_string(p, key);
    *(*p)++ = 0x02; /* AMF_STRING */
    rtmp_put_amf_string(p, v);
}

/* ------------------------------------------------------------------ */
/* RTMP Handshake                                                     */
/* ------------------------------------------------------------------ */

static int rtmp_handshake(int fd)
{
    uint8_t c0 = 0x03; /* RTMP version 3 */
    uint8_t c1[1536];
    memset(c1, 0, sizeof(c1));
    c1[0] = 0; /* timestamp */
    /* fill rest with random */
    for (int i = 1; i < 1536; i++) c1[i] = (uint8_t)esp_random();

    /* Send C0 + C1 */
    uint8_t c0c1[1537];
    c0c1[0] = c0;
    memcpy(c0c1 + 1, c1, 1536);
    if (rtmp_write(fd, c0c1, 1537) < 0) {
        ESP_LOGE(TAG, "RTMP handshake: send C0+C1 failed: errno %d", errno);
        return -1;
    }

    /* Receive S0 + S1 + S2 (1 + 1536 + 1536 = 3073 bytes) */
    uint8_t s0s1s2[3073];
    int total = 0;
    while (total < 3073) {
        int r = recv(fd, s0s1s2 + total, 3073 - total, 0);
        if (r < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
            ESP_LOGE(TAG, "RTMP handshake: recv S0+S1+S2 failed: errno %d", errno);
            return -1;
        }
        if (r == 0) {
            ESP_LOGE(TAG, "RTMP handshake: server closed connection");
            return -1;
        }
        total += r;
    }

    ESP_LOGI(TAG, "RTMP handshake complete (S0+S1+S2 received, 3073 bytes)");

    /* Send C2: copy of S1 (server's first 1536 bytes after S0) */
    /* This acknowledges the server's handshake and completes the RTMP handshake */
    if (rtmp_write(fd, s0s1s2 + 1, 1536) < 0) {
        ESP_LOGE(TAG, "RTMP handshake: send C2 failed: errno %d", errno);
        return -1;
    }
    ESP_LOGI(TAG, "RTMP handshake: C2 sent (1536 bytes), handshake complete");

    /* Send C2: copy of S1 (server's first 1536 bytes after S0) */
    /* This acknowledges the server's handshake and completes the RTMP handshake */
    if (rtmp_write(fd, s0s1s2 + 1, 1536) < 0) {
        ESP_LOGE(TAG, "RTMP handshake: send C2 failed: errno %d", errno);
        return -1;
    }
    ESP_LOGD(TAG, "RTMP handshake: C2 sent");

    return 0;
}

/* ------------------------------------------------------------------ */
/* RTMP Chunk Writing                                                 */
/* ------------------------------------------------------------------ */

static int rtmp_write_chunk(int fd, uint8_t chunk_type, uint32_t chunk_stream_id,
                            uint32_t timestamp, const uint8_t *data, int len)
{
    uint8_t header[13];
    int hlen = 0;

    /* Chunk Type 0: 11-byte header (fmt=0, csid follows) */
    header[0] = (0 << 6) | chunk_stream_id;  /* fmt=0 for first chunk */
    hlen = 1;

    /* Timestamp (3 bytes) */
    if (timestamp < 0xFFFFFF) {
        rtmp_put_be24(&header[hlen], timestamp);
    } else {
        rtmp_put_be24(&header[hlen], 0xFFFFFF);
    }
    hlen += 3;

    /* Body length (3 bytes) */
    rtmp_put_be24(&header[hlen], len);
    hlen += 3;

    /* Type (1 byte) - 0x09 = video */
    header[hlen++] = 0x09;

    /* Stream ID (4 bytes, little-endian) - ffmpeg uses server-assigned stream_id
     from createStream _result for video data chunk headers */
    header[hlen++] = s_rtmp.stream_id & 0xFF;
    header[hlen++] = (s_rtmp.stream_id >> 8) & 0xFF;
    header[hlen++] = (s_rtmp.stream_id >> 16) & 0xFF;
    header[hlen++] = (s_rtmp.stream_id >> 24) & 0xFF;

    /* Extended timestamp (1 byte, only when timestamp >= 0xFFFFFF) */
    if (timestamp >= 0xFFFFFF) {
        header[hlen++] = (timestamp >> 24) & 0xFF;
    }

    /* Write header */
    if (rtmp_write(fd, header, hlen) < 0) return -1;

    /* Write body in chunks */
    int offset = 0;
    while (offset < len) {
        int chunk_len = len - offset;
        if (s_rtmp.chunk_size > 0 && chunk_len > (int)s_rtmp.chunk_size) {
            chunk_len = s_rtmp.chunk_size;
        }
        if (rtmp_write(fd, data + offset, chunk_len) < 0) return -1;
        offset += chunk_len;

        /* If chunked, send subsequent chunk header (fmt=2: timestamp + csid only) */
        if (offset < len) {
            uint8_t ext_header[5];
            ext_header[0] = (2 << 6) | chunk_stream_id;  /* fmt=2, csid */
            uint32_t ext_ts = timestamp;
            if (ext_ts >= 0xFFFFFF) ext_ts = 0xFFFFFF;
            rtmp_put_be24(&ext_header[1], ext_ts);
            /* Extended timestamp byte for fmt=2 */
            if (timestamp >= 0xFFFFFF) {
                ext_header[4] = (timestamp >> 24) & 0xFF;
                if (rtmp_write(fd, ext_header, 5) < 0) return -1;
            } else {
                if (rtmp_write(fd, ext_header, 4) < 0) return -1;
            }
        }
    }

    return 0;
}

/* Send SetChunkSize command (type 0x01, csid 2) to negotiate chunk size */
static int rtmp_send_set_chunk_size(int fd, uint32_t chunk_size)
{
    uint8_t buf[15];
    int hlen = 0;
    buf[hlen++] = (0 << 6) | 2; /* fmt=0, csid=2 */
    rtmp_put_be24(&buf[hlen], 0); hlen += 3; /* timestamp */
    rtmp_put_be24(&buf[hlen], 4); hlen += 3; /* body length */
    buf[hlen++] = 0x01; /* type = Set Chunk Size */
    rtmp_put_be24(&buf[hlen], 0); hlen += 3; /* stream ID = 0 */
    buf[hlen++] = (chunk_size >> 24) & 0xFF;
    buf[hlen++] = (chunk_size >> 16) & 0xFF;
    buf[hlen++] = (chunk_size >> 8) & 0xFF;
    buf[hlen++] = chunk_size & 0xFF;
    return rtmp_write(fd, buf, hlen);
}

/* ------------------------------------------------------------------ */
/* RTMP Control Messages (following ffmpeg's sequence)                */
/* ------------------------------------------------------------------ */

/* RTMP message types (per ffmpeg rtmp.h):
 * 0x01 = Set Chunk Size
 * 0x04 = User Control
 * 0x05 = Window Acknowledgement Size
 * 0x06 = Set Peer Bandwidth
 * 0x09 = Video Message
 * 0x14 = Invoke
 */

/* Send Window Acknowledgement Size (type 0x05, csid 2) */
static int rtmp_send_window_ack_size(int fd, uint32_t size)
{
    uint8_t buf[15];
    int hlen = 0;
    buf[hlen++] = (0 << 6) | 2; /* fmt=0, csid=2 */
    rtmp_put_be24(&buf[hlen], 0); hlen += 3;
    rtmp_put_be24(&buf[hlen], 4); hlen += 3;
    buf[hlen++] = 0x05; /* type = Window Acknowledgement Size */
    rtmp_put_be24(&buf[hlen], 0); hlen += 3;
    buf[hlen++] = (size >> 24) & 0xFF;
    buf[hlen++] = (size >> 16) & 0xFF;
    buf[hlen++] = (size >> 8) & 0xFF;
    buf[hlen++] = size & 0xFF;
    return rtmp_write(fd, buf, hlen);
}

/* Send Set Peer Bandwidth (type 0x06, csid 2) */
static int rtmp_send_set_peer_bw(int fd, uint32_t size)
{
    uint8_t buf[15];
    int hlen = 0;
    buf[hlen++] = (0 << 6) | 2;
    rtmp_put_be24(&buf[hlen], 0); hlen += 3;
    rtmp_put_be24(&buf[hlen], 5); hlen += 3;
    buf[hlen++] = 0x06; /* type = Set Peer Bandwidth */
    rtmp_put_be24(&buf[hlen], 0); hlen += 3;
    buf[hlen++] = (size >> 24) & 0xFF;
    buf[hlen++] = (size >> 16) & 0xFF;
    buf[hlen++] = (size >> 8) & 0xFF;
    buf[hlen++] = size & 0xFF;
    buf[hlen++] = 0x02; /* limit type: dynamic */
    return rtmp_write(fd, buf, hlen);
}

/* Send User Control Stream Begin (type 0x04, csid 2) */
static int rtmp_send_user_control_stream_begin(int fd, uint32_t stream_id)
{
    uint8_t buf[15];
    int hlen = 0;
    buf[hlen++] = (0 << 6) | 2;
    rtmp_put_be24(&buf[hlen], 0); hlen += 3;
    rtmp_put_be24(&buf[hlen], 6); hlen += 3;
    buf[hlen++] = 0x04; /* type = User Control */
    rtmp_put_be24(&buf[hlen], 0); hlen += 3;
    buf[hlen++] = 0x00; /* event = 0 (Stream Begin) */
    buf[hlen++] = 0x00;
    buf[hlen++] = (stream_id >> 24) & 0xFF;
    buf[hlen++] = (stream_id >> 16) & 0xFF;
    buf[hlen++] = (stream_id >> 8) & 0xFF;
    buf[hlen++] = stream_id & 0xFF;
    return rtmp_write(fd, buf, hlen);
}

/* Send _result for connect (type 0x14 = INVOKE, csid 3) */
static int rtmp_send_connect_result(int fd)
{
    uint8_t buf[256];
    uint8_t *p = buf;

    memcpy(p, "_result", 7); p += 7;
    *p++ = 0x00; /* AMF_NUMBER */
    rtmp_put_be64(p, 1.0); /* transaction ID */
    p += 8;
    *p++ = 0x05; /* AMF_NULL */
    *p++ = 0x03; /* AMF_OBJECT */
    rtmp_put_amf_string_prop(&p, "fmsVer", "FMS/3,0,1,123");
    rtmp_put_amf_number_prop(&p, "capabilities", 31.0);
    rtmp_put_amf_object_end(&p);

    *p++ = 0x03; /* AMF_OBJECT */
    rtmp_put_amf_string_prop(&p, "level", "status");
    rtmp_put_amf_string_prop(&p, "code", "NetConnection.Connect.Success");
    rtmp_put_amf_string_prop(&p, "description", "Connection succeeded.");
    rtmp_put_amf_number_prop(&p, "objectEncoding", 0.0);
    rtmp_put_amf_object_end(&p);

    /* RTMP chunk header */
    uint8_t header[12];
    int hlen = 0;
    header[hlen++] = (0 << 6) | 3;
    rtmp_put_be24(&header[hlen], 0); hlen += 3;
    rtmp_put_be24(&header[hlen], (uint32_t)(p - buf)); hlen += 3;
    header[hlen++] = 0x14; /* INVOKE */
    header[hlen++] = 0; header[hlen++] = 0; header[hlen++] = 0; header[hlen++] = 0;

    uint8_t combined[12 + 256];
    memcpy(combined, header, hlen);
    memcpy(combined + hlen, buf, p - buf);

    return rtmp_write(fd, combined, hlen + (int)(p - buf));
}

/* Send onBWDone (type 0x14 = INVOKE, csid 3) */
static int rtmp_send_on_bw_done(int fd)
{
    uint8_t buf[64];
    uint8_t *p = buf;

    memcpy(p, "onBWDone", 8); p += 8;
    *p++ = 0x00; /* AMF_NUMBER */
    rtmp_put_be64(p, 0.0);
    p += 8;
    *p++ = 0x05; /* AMF_NULL */
    rtmp_put_be64(p, 8192.0);
    p += 8;

    /* RTMP chunk header */
    uint8_t header[12];
    int hlen = 0;
    header[hlen++] = (0 << 6) | 3;
    rtmp_put_be24(&header[hlen], 0); hlen += 3;
    rtmp_put_be24(&header[hlen], (uint32_t)(p - buf)); hlen += 3;
    header[hlen++] = 0x14; /* INVOKE */
    header[hlen++] = 0; header[hlen++] = 0; header[hlen++] = 0; header[hlen++] = 0;

    uint8_t combined[12 + 64];
    memcpy(combined, header, hlen);
    memcpy(combined + hlen, buf, p - buf);

    return rtmp_write(fd, combined, hlen + (int)(p - buf));
}

/* ------------------------------------------------------------------ */
/* RTMP AMF Commands                                                  */
/* ------------------------------------------------------------------ */

static int rtmp_send_connect(int fd)
{
    uint8_t buf[512];
    uint8_t *p = buf;

    /* Command name */
    memcpy(p, "connect", 7); p += 7;

    /* Transaction ID (number) */
    *p++ = 0x00; /* AMF_NUMBER */
    rtmp_put_be64(p, 1.0);

    /* Object */
    *p++ = 0x03; /* AMF_OBJECT */
    rtmp_put_amf_string_prop(&p, "app", s_rtmp.app);
    rtmp_put_amf_string_prop(&p, "flashVer", "LNX 9,0,124,2");
    rtmp_put_amf_string_prop(&p, "swfUrl", "");

    /* tcUrl format per ffmpeg (rtmp://host:port/app) - NO credentials, NO stream.
     * Credentials sent as separate AMF properties in connect message.
     * Stream name sent separately in publish command.
     * For MediaMTX compatibility, we also support query-param auth format. */
    char tcUrl[256];
    int tcUrlLen;
    if (s_rtmp.pass[0] != '\0') {
        /* MediaMTX prefers query-param auth: rtmp://host:port/app/stream?user=xxx&pass=xxx */
        tcUrlLen = snprintf(tcUrl, sizeof(tcUrl),
            "rtmp://%s:%d/%s/%s?user=%s&pass=%s",
            s_rtmp.server, s_rtmp.port, s_rtmp.app, s_rtmp.stream,
            s_rtmp.user, s_rtmp.pass);
    } else if (s_rtmp.user[0] != '\0') {
        /* User only, no password */
        tcUrlLen = snprintf(tcUrl, sizeof(tcUrl),
            "rtmp://%s:%d/%s/%s?user=%s",
            s_rtmp.server, s_rtmp.port, s_rtmp.app, s_rtmp.stream,
            s_rtmp.user);
    } else {
        /* No credentials in URL */
        tcUrlLen = snprintf(tcUrl, sizeof(tcUrl),
            "rtmp://%s:%d/%s/%s",
            s_rtmp.server, s_rtmp.port, s_rtmp.app, s_rtmp.stream);
    }
    if (tcUrlLen > 0 && tcUrlLen < (int)sizeof(tcUrl)) {
        rtmp_put_amf_string_prop(&p, "tcUrl", tcUrl);
    } else {
        rtmp_put_amf_string_prop(&p, "tcUrl", s_rtmp.app);
    }

    /* Explicit user/pass in connect message (some RTMP servers require this) */
    if (s_rtmp.user[0] != '\0') {
        rtmp_put_amf_string_prop(&p, "user", s_rtmp.user);
    }
    if (s_rtmp.pass[0] != '\0') {
        rtmp_put_amf_string_prop(&p, "pass", s_rtmp.pass);
    }

    rtmp_put_amf_object_prop(&p, "fpad", 0.0);
    rtmp_put_amf_object_prop(&p, "capabilities", 239.0);
    rtmp_put_amf_object_prop(&p, "audioCodecs", 3191.0);
    rtmp_put_amf_object_prop(&p, "videoCodecs", 252.0);
    rtmp_put_amf_object_prop(&p, "videoFunction", 1.0);
    rtmp_put_amf_object_prop(&p, "objectEncoding", 0.0);
    rtmp_put_amf_string_prop(&p, "type", "nonprivate");
    rtmp_put_amf_object_end(&p);

    return rtmp_write_chunk(fd, 0, RTMP_CHUNK_INVOKE, 0, buf, (int)(p - buf));
}

static int rtmp_send_create_stream(int fd)
{
    uint8_t buf[128];
    uint8_t *p = buf;

    memcpy(p, "createStream", 12); p += 12;

    *p++ = 0x00; /* AMF_NUMBER */
    rtmp_put_be64(p, 2.0);

    *p++ = 0x05; /* AMF_NULL */

    return rtmp_write_chunk(fd, 0, RTMP_CHUNK_INVOKE, 0, buf, (int)(p - buf));
}

static int rtmp_send_publish(int fd)
{
    uint8_t buf[256];
    uint8_t *p = buf;

    memcpy(p, "publish", 7); p += 7;

    *p++ = 0x00; /* AMF_NUMBER */
    rtmp_put_be64(p, 3.0);

    *p++ = 0x05; /* AMF_NULL */

    rtmp_put_amf_string(&p, s_rtmp.stream);
    /* Publish with just stream name (credentials go in tcUrl during connect,
     * matching ffmpeg's behavior with MediaMTX) */
    rtmp_put_amf_string(&p, "live");

    /* ffmpeg uses RTMP_SOURCE_CHANNEL (4) for publish, not RTMP_CHUNK_INVOKE (3) */
    return rtmp_write_chunk(fd, 0, RTMP_CHUNK_SOURCE, 0, buf, (int)(p - buf));
}

/* Send releaseStream (csid 3) */
static int rtmp_send_release_stream(int fd)
{
    uint8_t buf[256];
    uint8_t *p = buf;

    memcpy(p, "releaseStream", 13); p += 13;
    *p++ = 0x00; /* AMF_NUMBER */
    rtmp_put_be64(p, 4.0);
    *p++ = 0x05; /* AMF_NULL */
    rtmp_put_amf_string(&p, s_rtmp.stream);

    return rtmp_write_chunk(fd, 0, RTMP_CHUNK_INVOKE, 0, buf, (int)(p - buf));
}

/* Send FCPublish (csid 3) */
static int rtmp_send_fc_publish(int fd)
{
    uint8_t buf[256];
    uint8_t *p = buf;

    memcpy(p, "FCPublish", 9); p += 9;
    *p++ = 0x00; /* AMF_NUMBER */
    rtmp_put_be64(p, 5.0);
    *p++ = 0x05; /* AMF_NULL */
    rtmp_put_amf_string(&p, s_rtmp.stream);

    return rtmp_write_chunk(fd, 0, RTMP_CHUNK_INVOKE, 0, buf, (int)(p - buf));
}

/* ------------------------------------------------------------------ */
/* Read _result response from createStream/publish to get stream ID     */
/* ------------------------------------------------------------------ */

static int rtmp_read_create_stream_response(int fd)
{
    uint8_t header[12];
    uint8_t buf[256];

    /* Read CDT word (1 byte) */
    if (rtmp_read(fd, &header[0], 1) < 0) {
        ESP_LOGE(TAG, "RTMP: read CDT word failed: errno %d", errno);
        return -1;
    }

    uint8_t fmt = (header[0] >> 6) & 0x03;
    uint8_t csid = header[0] & 0x3F;

    /* Accept any CSID - MediaMTX may send _result on any channel */
    ESP_LOGD(TAG, "RTMP: read response: fmt=%d csid=%d", fmt, csid);

    /* Read timestamp (3 bytes) - only for fmt=0,1 */
    if (fmt < 3) {
        if (rtmp_read(fd, &header[1], 3) < 0) {
            ESP_LOGE(TAG, "RTMP: read timestamp failed: errno %d", errno);
            return -1;
        }
    }

    /* Read body length (3 bytes) - only for fmt=0 */
    int body_len = 0;
    if (fmt == 0) {
        if (rtmp_read(fd, &header[4], 3) < 0) {
            ESP_LOGE(TAG, "RTMP: read body length failed: errno %d", errno);
            return -1;
        }
        body_len = (header[4] << 16) | (header[5] << 8) | header[6];
    }

    /* Read message type (1 byte) - only for fmt=0 */
    if (fmt == 0) {
        if (rtmp_read(fd, &header[7], 1) < 0) {
            ESP_LOGE(TAG, "RTMP: read message type failed: errno %d", errno);
            return -1;
        }
    }

    /* Read stream ID (4 bytes LE) - only for fmt=0 */
    if (fmt == 0) {
        if (rtmp_read(fd, &header[8], 4) < 0) {
            ESP_LOGE(TAG, "RTMP: read stream ID failed: errno %d", errno);
            return -1;
        }
    }

    /* Read AMF data (body_len bytes) */
    if (body_len > (int)sizeof(buf)) {
        ESP_LOGE(TAG, "RTMP: body too large: %d", body_len);
        return -1;
    }
    if (rtmp_read(fd, buf, body_len) < 0) {
        ESP_LOGE(TAG, "RTMP: read AMF data failed: errno %d", errno);
        return -1;
    }

    /* Parse AMF: "command_name" (string), transaction_id (number), null, object */
    uint8_t *p = buf;

    /* Command name string */
    if (*p != 0x02) { /* AMF_STRING */
        ESP_LOGE(TAG, "RTMP: expected AMF_STRING for command name, got 0x%02x", *p);
        return -1;
    }
    p++; /* skip type */
    int cmd_name_len = (p[0] << 8) | p[1];
    p += 2;
    if (cmd_name_len != 7 || memcmp(p, "_result", 7) != 0) {
        ESP_LOGE(TAG, "RTMP: expected _result command, got %.*s", cmd_name_len, p);
        return -1;
    }
    p += cmd_name_len;

    /* Transaction ID (number) */
    if (*p != 0x00) { /* AMF_NUMBER */
        ESP_LOGE(TAG, "RTMP: expected AMF_NUMBER for transaction ID, got 0x%02x", *p);
        return -1;
    }
    p++; /* skip type */
    p += 8; /* skip 8-byte double */

    /* Null */
    if (*p != 0x05) { /* AMF_NULL */
        ESP_LOGE(TAG, "RTMP: expected AMF_NULL, got 0x%02x", *p);
        return -1;
    }
    p++; /* skip type */

    /* Object */
    if (*p != 0x03) { /* AMF_OBJECT */
        ESP_LOGE(TAG, "RTMP: expected AMF_OBJECT, got 0x%02x", *p);
        return -1;
    }
    p++; /* skip type */

    /* Read object properties until object end (0x00 0x00 0x09) */
    while (p < buf + body_len) {
        /* Check for object end */
        if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x09) {
            p += 3;
            break;
        }

        /* Read property name */
        int name_len = (p[0] << 8) | p[1];
        p += 2;
        char name[64];
        if (name_len > (int)sizeof(name)) {
            p += name_len;
            continue;
        }
        memcpy(name, p, name_len);
        name[name_len] = '\0';
        p += name_len;

        /* Read property value */
        uint8_t type = *p;
        p++;

        if (strcmp(name, "streamId") == 0) {
            if (type == 0x00) { /* AMF_NUMBER */
                uint8_t *p_start = p;
                uint64_t bits = 0;
                for (int i = 0; i < 8; i++) {
                    bits = (bits << 8) | *p_start++;
                }
                double d;
                memcpy(&d, &bits, 8);
                s_rtmp.stream_id = (uint32_t)d;
                ESP_LOGI(TAG, "RTMP: stream ID from server: %u", s_rtmp.stream_id);
                return 0;
            } else {
                /* Skip unknown type */
                if (type == 0x02) { /* AMF_STRING */
                    int str_len = (p[0] << 8) | p[1];
                    p += 2 + str_len;
                } else if (type == 0x00) { /* AMF_NUMBER */
                    p += 8;
                } else if (type == 0x01) { /* AMF_BOOLEAN */
                    p += 1;
                } else if (type == 0x05) { /* AMF_NULL */
                    /* nothing */
                } else {
                    ESP_LOGE(TAG, "RTMP: unknown AMF object property type 0x%02x", type);
                    return -1;
                }
            }
        } else {
            /* Skip unknown property */
            if (type == 0x02) { /* AMF_STRING */
                int str_len = (p[0] << 8) | p[1];
                p += 2 + str_len;
            } else if (type == 0x00) { /* AMF_NUMBER */
                p += 8;
            } else if (type == 0x01) { /* AMF_BOOLEAN */
                p += 1;
            } else if (type == 0x05) { /* AMF_NULL */
                /* nothing */
            } else {
                ESP_LOGE(TAG, "RTMP: unknown AMF object property type 0x%02x", type);
                return -1;
            }
        }
    }

    ESP_LOGE(TAG, "RTMP: streamId not found in handshake response");
    return -1;
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

void network_tx_rtmp_config_default(void)
{
    memset(&s_rtmp, 0, sizeof(s_rtmp));
    strncpy(s_rtmp.server, "172.160.27.93", sizeof(s_rtmp.server) - 1);
    s_rtmp.port = 1935;
    strncpy(s_rtmp.app, "live", sizeof(s_rtmp.app) - 1);
    strncpy(s_rtmp.stream, "sse_drone_f71919", sizeof(s_rtmp.stream) - 1);
    strncpy(s_rtmp.user, "sse_drone_f71919", sizeof(s_rtmp.user) - 1);
    strncpy(s_rtmp.pass, "5eefeb2389cf8b2ac83218ae926e7143", sizeof(s_rtmp.pass) - 1);
    s_rtmp.chunk_size = RTMP_CHUNK_SIZE;
    s_rtmp.timestamp = 0;
    s_rtmp.stream_id = 0;
}

void network_tx_set_rtmp_config(const char *server, uint16_t port,
                                const char *app, const char *stream,
                                const char *user, const char *pass)
{
    if (server) strncpy(s_rtmp.server, server, sizeof(s_rtmp.server) - 1);
    s_rtmp.port = port;
    if (app) strncpy(s_rtmp.app, app, sizeof(s_rtmp.app) - 1);
    if (stream) strncpy(s_rtmp.stream, stream, sizeof(s_rtmp.stream) - 1);
    if (user) strncpy(s_rtmp.user, user, sizeof(s_rtmp.user) - 1);
    if (pass) strncpy(s_rtmp.pass, pass, sizeof(s_rtmp.pass) - 1);
}

esp_err_t network_tx_rtmp_module_init(void)
{
    /* Apply defaults if no config was set via network_tx_set_rtmp_config() */
    if (s_rtmp.server[0] == '\0') {
        network_tx_rtmp_config_default();
    }
    if (s_rtmp.connected) {
        ESP_LOGW(TAG, "RTMP already connected");
        return ESP_OK;
    }

    /* Try alternative stream IDs if MediaMTX rejects the primary one */
    for (int attempt = 0; attempt < S_RTMP_ALT_STREAMS_COUNT; attempt++) {
        if (attempt > 0) {
            rtmp_next_stream_id();
        }

        ESP_LOGI(TAG, "RTMP: connecting to %s:%d app=%s stream=%s (attempt %d/%d)",
                 s_rtmp.server, s_rtmp.port, s_rtmp.app, s_rtmp.stream,
                 attempt + 1, S_RTMP_ALT_STREAMS_COUNT);

        /* Create TCP socket */
        int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (fd < 0) {
            ESP_LOGE(TAG, "RTMP: socket() failed: errno %d", errno);
            return ESP_FAIL;
        }

        /* Set non-blocking */
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        /* Connect to server */
        struct sockaddr_in dst;
        memset(&dst, 0, sizeof(dst));
        dst.sin_family = AF_INET;
        dst.sin_port = htons(s_rtmp.port);
        if (inet_pton(AF_INET, s_rtmp.server, &dst.sin_addr) <= 0) {
            ESP_LOGE(TAG, "RTMP: inet_pton() failed for %s", s_rtmp.server);
            close(fd);
            return ESP_FAIL;
        }

        if (connect(fd, (struct sockaddr *)&dst, sizeof(dst)) < 0) {
            if (errno != EINPROGRESS) {
                ESP_LOGE(TAG, "RTMP: connect() failed immediately: errno %d (%s)", errno, strerror(errno));
                close(fd);
                continue;  /* Try next stream ID */
            }
            /* Wait for connect to complete */
            struct timeval tv;
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

            /* Wait for connect to complete with select() for better diagnostics */
            fd_set write_fds, error_fds;
            FD_ZERO(&write_fds);
            FD_ZERO(&error_fds);
            FD_SET(fd, &write_fds);
            FD_SET(fd, &error_fds);

            struct timeval connect_timeout;
            connect_timeout.tv_sec = 10;
            connect_timeout.tv_usec = 0;

            ESP_LOGI(TAG, "RTMP: waiting for TCP connect to complete (non-blocking)...");
            int sel_ret = select(fd + 1, NULL, &write_fds, &error_fds, &connect_timeout);
            if (sel_ret <= 0) {
                if (sel_ret == 0) {
                    ESP_LOGE(TAG, "RTMP: connect() timed out after %d seconds (errno 118 ETIMEDOUT)", connect_timeout.tv_sec);
                } else {
                    ESP_LOGE(TAG, "RTMP: select() failed: errno %d (%s)", errno, strerror(errno));
                }
                close(fd);
                continue;  /* Try next stream ID */
            }

            /* Check for errors */
            if (FD_ISSET(fd, &error_fds)) {
                int so_error = 0;
                socklen_t len = sizeof(so_error);
                getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
                ESP_LOGE(TAG, "RTMP: connect() failed with socket error: %d (%s)", so_error, strerror(so_error));
                close(fd);
                continue;  /* Try next stream ID */
            }

            if (!FD_ISSET(fd, &write_fds)) {
                ESP_LOGE(TAG, "RTMP: select() returned but fd not writable");
                close(fd);
                continue;  /* Try next stream ID */
            }

            ESP_LOGI(TAG, "RTMP: TCP connect succeeded, proceeding to RTMP handshake");
        }

        /* Set socket timeouts for RTMP handshake and data exchange */
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        int one = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

        /* RTMP handshake */
        if (rtmp_handshake(fd) < 0) {
            close(fd);
            continue;  /* Try next stream ID */
        }

        /* Switch to blocking mode after handshake */
        flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

        /* Connect command (csid 3) */
        ESP_LOGI(TAG, "RTMP: sending connect command (app=%s, tcUrl=rtmp://%s:%d/%s/%s)...",
                 s_rtmp.app, s_rtmp.server, s_rtmp.port, s_rtmp.app, s_rtmp.stream);
        if (rtmp_send_connect(fd) < 0) {
            ESP_LOGE(TAG, "RTMP: connect command failed");
            close(fd);
            continue;  /* Try next stream ID */
        }
        ESP_LOGI(TAG, "RTMP: connect command sent, waiting for _result...");

        /* Read ALL server responses to connect:
           Window Ack Size, Set Peer BW, User Control, Chunk Size,
           _result for connect, onBWDone */
        ESP_LOGI(TAG, "RTMP: reading server responses to connect command...");
        rtmp_drain_response(fd);
        ESP_LOGI(TAG, "RTMP: server responses to connect processed");

        /* Client-side control messages per ffmpeg's read_connect() sequence */
        if (rtmp_send_window_ack_size(fd, s_rtmp.chunk_size) < 0) {
            ESP_LOGE(TAG, "RTMP: Window Ack Size failed");
            close(fd);
            continue;
        }
        rtmp_drain_response(fd);

        if (rtmp_send_set_peer_bw(fd, s_rtmp.chunk_size) < 0) {
            ESP_LOGE(TAG, "RTMP: Set Peer BW failed");
            close(fd);
            continue;
        }
        rtmp_drain_response(fd);

        if (rtmp_send_user_control_stream_begin(fd, 0) < 0) {
            ESP_LOGE(TAG, "RTMP: User Control Stream Begin failed");
            close(fd);
            continue;
        }
        rtmp_drain_response(fd);

        if (rtmp_send_set_chunk_size(fd, s_rtmp.chunk_size) < 0) {
            ESP_LOGE(TAG, "RTMP: Set Chunk Size failed");
            close(fd);
            continue;
        }
        rtmp_drain_response(fd);

        /* ffmpeg sends releaseStream and FCPublish before createStream */
        ESP_LOGI(TAG, "RTMP: sending releaseStream for stream '%s'...", s_rtmp.stream);
        if (rtmp_send_release_stream(fd) < 0) {
            ESP_LOGE(TAG, "RTMP: releaseStream failed");
            close(fd);
            continue;  /* Try next stream ID */
        }
        ESP_LOGI(TAG, "RTMP: releaseStream sent");
        rtmp_drain_response(fd);
        ESP_LOGI(TAG, "RTMP: releaseStream response processed");

        ESP_LOGI(TAG, "RTMP: sending FCPublish for stream '%s'...", s_rtmp.stream);
        if (rtmp_send_fc_publish(fd) < 0) {
            ESP_LOGE(TAG, "RTMP: FCPublish failed");
            close(fd);
            continue;  /* Try next stream ID */
        }
        ESP_LOGI(TAG, "RTMP: FCPublish sent");
        rtmp_drain_response(fd);
        ESP_LOGI(TAG, "RTMP: FCPublish response processed");

        /* CreateStream */
        ESP_LOGI(TAG, "RTMP: sending createStream command...");
        if (rtmp_send_create_stream(fd) < 0) {
            ESP_LOGE(TAG, "RTMP: createStream command failed");
            close(fd);
            continue;  /* Try next stream ID */
        }
        ESP_LOGI(TAG, "RTMP: createStream command sent, waiting for _result with stream ID...");

        /* Read _result for createStream to extract server-assigned stream ID */
        esp_err_t err = rtmp_read_create_stream_response(fd);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "RTMP: createStream succeeded, server-assigned stream_id=%d", s_rtmp.stream_id);
        } else {
            ESP_LOGE(TAG, "Failed to read stream_id from createStream response, falling back to 1");
            s_rtmp.stream_id = 1;
        }

        /* Read any remaining createStream response data */
        rtmp_drain_response(fd);
        ESP_LOGI(TAG, "RTMP: createStream response processed");

        /* Publish */
        ESP_LOGI(TAG, "RTMP: sending publish command for stream '%s' (type=live)...", s_rtmp.stream);
        if (rtmp_send_publish(fd) < 0) {
            ESP_LOGE(TAG, "RTMP: publish command failed");
            close(fd);
            continue;  /* Try next stream ID */
        }
        ESP_LOGI(TAG, "RTMP: publish command sent, waiting for onStatus...");

        /* Read MediaMTX response to publish (_result + onStatus NetStream.Publish.Start) */
        rtmp_drain_response(fd);
        ESP_LOGI(TAG, "RTMP: publish response processed");

        s_rtmp.tcp_fd = fd;
        s_rtmp.connected = true;
        s_rtmp.stream_published = true;
        s_rtmp.timestamp = 0;
        s_rtmp.mutex = xSemaphoreCreateMutex();

        ESP_LOGI(TAG, "RTMP: connected and publishing to %s/%s",
                 s_rtmp.app, s_rtmp.stream);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "RTMP: all stream ID attempts failed");
    return ESP_FAIL;
}

bool network_tx_rtmp_is_connected(void)
{
    return s_rtmp.connected;
}

void network_tx_rtmp_deinit(void)
{
    if (s_rtmp.tcp_fd >= 0) {
        close(s_rtmp.tcp_fd);
        s_rtmp.tcp_fd = -1;
    }
    if (s_rtmp.mutex) {
        vSemaphoreDelete(s_rtmp.mutex);
        s_rtmp.mutex = NULL;
    }
    s_rtmp.connected = false;
    s_rtmp.stream_published = false;
    s_rtmp.avc_config_sent = false;
    s_rtmp.metadata_sent = false;

    /* Reset encoder SPS/PPS flag so next RTMP session sends SPS/PPS in first frame */
    h264_encoder_reset_sps_pps();
}



/* ------------------------------------------------------------------ */
/* RTMP Video Tag                                                     */
/* ------------------------------------------------------------------ */

/* Send FLV-wrapped H.264 NAL data as RTMP video message (type 0x09).
 * FLV video tag format:
 *   Byte 0: 0x09 (tag type: video)
 *   Bytes 1-3: data size (big-endian, includes FLV video body header + NAL data)
 *   Bytes 4-6: timestamp (big-endian)
 *   Byte 7: timestamp extended (0)
 *   Bytes 8-10: stream ID (0)
 * FLV video body:
 *   Byte 0: 0x17 (keyframe + AVC codec)
 *   Bytes 1-3: composition time offset (0x000000)
 *   Bytes 4+: NAL unit data */
static int rtmp_send_video_tag(int fd, const uint8_t *data, int len,
                               uint32_t timestamp_ms, uint8_t avc_packet_type)
{
    /* Send FLV-wrapped H.264 frame data as SINGLE RTMP video message (type 0x09).
     * Input data contains concatenated NAL units (SPS+PPS+AUD+IDR or AUD+IDR).
     * FLV video tag format:
     *   Byte 0: 0x09 (tag type: video)
     *   Bytes 1-3: data size (big-endian, includes FLV video body header + ALL NAL data)
     *   Bytes 4-6: timestamp (big-endian)
     *   Byte 7: timestamp extended (0)
     *   Bytes 8-10: stream ID (0)
     * FLV video body (AVC/CodecID=7):
     *   Byte 0: FrameType (4 bits) | CodecID (4 bits) = 0x17 (1=keyframe, 7=AVC)
     *   Byte 1: AVCPacketType: 0=Sequence Header, 1=NALU, 2=End of Sequence
     *   Bytes 2-4: CompositionTimeOffset (24-bit, big-endian)
     *   Bytes 5+: Payload:
     *          - If AVCPacketType=0: AVCDecoderConfigurationRecord (contains SPS/PPS)
     *          - If AVCPacketType=1: NAL units with 4-byte length prefixes (NO start codes) */
    if (!s_rtmp.connected) {
        ESP_LOGW(TAG, "RTMP: not connected, dropping video frame (len=%d)", len);
        return -1;
    }

    ESP_LOGD(TAG, "RTMP: sending video frame (type=%d, len=%d, timestamp=%u, avc_type=%d)",
             0x09, len, timestamp_ms, avc_packet_type);

    /* Send FLV-wrapped H.264 frame data as SINGLE RTMP video message (type 0x09).
     * Input data contains concatenated NAL units (SPS+PPS+AUD+IDR or AUD+IDR).
     * FLV video tag format:
     *   Byte 0: 0x09 (tag type: video)
     *   Bytes 1-3: data size (big-endian, includes FLV video body header + ALL NAL data)
     *   Bytes 4-6: timestamp (big-endian)
     *   Byte 7: timestamp extended (0)
     *   Bytes 8-10: stream ID (0)
     * FLV video body (AVC/CodecID=7):
     *   Byte 0: FrameType (4 bits) | CodecID (4 bits) = 0x17 (1=keyframe, 7=AVC)
     *   Byte 1: AVCPacketType: 0=Sequence Header, 1=NALU, 2=End of Sequence
     *   Bytes 2-4: CompositionTimeOffset (24-bit, big-endian)
     *   Bytes 5+: Payload:
     *          - If AVCPacketType=0: AVCDecoderConfigurationRecord (contains SPS/PPS)
     *          - If AVCPacketType=1: NAL units with 4-byte length prefixes (NO start codes) */
    uint8_t flv_tag[11 + RTMP_CHUNK_SIZE];
    int hlen = 0;

    flv_tag[hlen++] = 0x09; /* tag type: video */
    /* data size = 5 (FLV video body header with AVCPacketType) + NAL data length */
    rtmp_put_be24(&flv_tag[hlen], 5 + len); hlen += 3; /* data size: FLV video body header (5) + NAL data */
    rtmp_put_be24(&flv_tag[hlen], timestamp_ms); hlen += 3; /* timestamp */
    flv_tag[hlen++] = 0; /* timestamp extended */
    rtmp_put_be24(&flv_tag[hlen], 0); hlen += 3; /* stream ID = 0 */

    /* FLV video body: 5 bytes (FrameType+CodecID, AVCPacketType, CompositionTime×3) */
    flv_tag[hlen++] = 0x17; /* keyframe + AVC */
    flv_tag[hlen++] = avc_packet_type; /* 0=sequence header, 1=NALU */
    flv_tag[hlen++] = 0x00; /* composition time offset */
    flv_tag[hlen++] = 0x00;
    flv_tag[hlen++] = 0x00;

    int copy_len = len < (int)(sizeof(flv_tag) - hlen) ? len : (int)(sizeof(flv_tag) - hlen);
    memcpy(flv_tag + hlen, data, copy_len);

    /* Total body length = FLV tag header (11) + FLV video body header (5) + ALL NAL data */
    int ret = rtmp_write_chunk(fd, 0, RTMP_CHUNK_VIDEO, timestamp_ms, flv_tag, 11 + 5 + len);
    if (ret < 0) {
        ESP_LOGE(TAG, "RTMP: failed to send video frame (len=%d, timestamp=%u): errno %d", len, timestamp_ms, errno);
    } else {
        ESP_LOGD(TAG, "RTMP: video frame sent successfully (len=%d, timestamp=%u)", len, timestamp_ms);
    }
    return ret;
}

/* ------------------------------------------------------------------ */
/* H.264 NAL extraction from TS packets                               */
/* ------------------------------------------------------------------ */

/* Find H.264 start code (0x00000001 or 0x000001) in TS data */
static int find_start_code(const uint8_t *data, int len, int offset)
{
    for (int i = offset; i + 3 < len; i++) {
        if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
            return i; /* 4-byte start code */
        }
        if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01) {
            return i; /* 3-byte start code */
        }
    }
    return -1;
}

/* Send onMetaData RTMP notify message (ffmpeg sends this before first video frame) */
static int rtmp_send_on_metadata(int fd)
{
    uint8_t buf[256];
    uint8_t *p = buf;

    /* Prefix with "@setDataFrame" as ffmpeg does for NOTIFY packets */
    memcpy(p, "@setDataFrame", 13); p += 13;

    /* AMF: "onMetaData" string */
    rtmp_put_amf_string(&p, "onMetaData");
    *p++ = 0x05; /* AMF_NULL */
    *p++ = 0x03; /* AMF_OBJECT */
    /* Metadata properties */
    rtmp_put_amf_object_prop(&p, "duration", 0.0);
    rtmp_put_amf_object_prop(&p, "width", 1280.0);
    rtmp_put_amf_object_prop(&p, "height", 720.0);
    rtmp_put_amf_object_prop(&p, "displayWidth", 1280.0);
    rtmp_put_amf_object_prop(&p, "displayHeight", 720.0);
    rtmp_put_amf_object_prop(&p, "framerate", 30.0);
    rtmp_put_amf_object_prop(&p, "videocodecid", 7.0);
    rtmp_put_amf_object_prop(&p, "videodatarate", 0.0);
    rtmp_put_amf_object_end(&p);

    /* Build RTMP chunk header: fmt=0, csid=4 (source), timestamp=0,
       length=p-buf, type=0x12 (NOTIFY), stream_id=server-assigned */
    uint8_t header[12];
    int hlen = 0;
    header[hlen++] = (0 << 6) | RTMP_CHUNK_SOURCE; /* fmt=0, csid=4 */
    rtmp_put_be24(&header[hlen], 0); hlen += 3; /* timestamp */
    rtmp_put_be24(&header[hlen], (uint32_t)(p - buf)); hlen += 3; /* body length */
    header[hlen++] = 0x12; /* type = NOTIFY */
    header[hlen++] = s_rtmp.stream_id & 0xFF;
    header[hlen++] = (s_rtmp.stream_id >> 8) & 0xFF;
    header[hlen++] = (s_rtmp.stream_id >> 16) & 0xFF;
    header[hlen++] = (s_rtmp.stream_id >> 24) & 0xFF;

    uint8_t combined[12 + 256];
    memcpy(combined, header, hlen);
    memcpy(combined + hlen, buf, p - buf);

    return rtmp_write(fd, combined, hlen + (int)(p - buf));
}

/* Build AVCDecoderConfigurationRecord from SPS/PPS NAL units */
static int rtmp_build_avc_config_record(const uint8_t *sps_data, int sps_len,
                                         const uint8_t *pps_data, int pps_len,
                                         uint8_t *out, int *out_len)
{
    if (!sps_data || !pps_data || !out || !out_len) return -1;
    if (sps_len < 4 || pps_len < 1) return -1;

    uint8_t *p = out;
    *p++ = 0x01; /* configurationVersion */
    *p++ = sps_data[1]; /* AVCProfileIndication */
    *p++ = sps_data[2]; /* profile_compatibility */
    *p++ = sps_data[3]; /* AVCLevelIndication */
    *p++ = 0xFF; /* lengthSizeMinusOne = 3 (4-byte NAL length) */
    *p++ = 0xE1; /* 6 reserved bits + 5 bits numOfSequenceParameterSets = 1 */
    rtmp_put_be16(p, sps_len);
    p += 2;
    memcpy(p, sps_data, sps_len);
    p += sps_len;
    *p++ = 0x01; /* numOfPictureParameterSets = 1 */
    rtmp_put_be16(p, pps_len);
    p += 2;
    memcpy(p, pps_data, pps_len);
    p += pps_len;

    *out_len = (int)(p - out);
    return 0;
}

/* Send AVC sequence header (AVCPacketType=0) with SPS/PPS */
static int rtmp_send_video_seq_header(int fd, const uint8_t *data, int len,
                                       uint32_t timestamp_ms)
{
    /* Parse SPS and PPS from concatenated NAL data */
    const uint8_t *sps_data = NULL;
    int sps_len = 0;
    const uint8_t *pps_data = NULL;
    int pps_len = 0;

    int offset = 0;
    while (offset < len - 4) {
        int start = -1;
        for (int i = offset; i + 3 < len; i++) {
            if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
                start = i; break;
            }
            if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01) {
                start = i; break;
            }
        }
        if (start < 0) break;

        int next = -1;
        for (int i = start + 4; i + 3 < len; i++) {
            if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
                next = i; break;
            }
            if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01) {
                next = i; break;
            }
        }
        int nal_end = (next < 0) ? len : next;

        int nal_code_len = 4;
        if (data[start+2] == 0x01) nal_code_len = 3;

        const uint8_t *nal = data + start + nal_code_len;
        int nal_size = nal_end - start - nal_code_len;

        if (nal_size > 0) {
            uint8_t nal_type = nal[0] & 0x1F;
            if (nal_type == 7 && !sps_data) { /* SPS */
                sps_data = nal;
                sps_len = nal_size;
            } else if (nal_type == 8 && !pps_data) { /* PPS */
                pps_data = nal;
                pps_len = nal_size;
            }
        }

        offset = nal_end;
        if (sps_data && pps_data) break;
    }

    if (!sps_data || !pps_data) {
        ESP_LOGW(TAG, "RTMP: SPS/PPS not found in first frame");
        return -1;
    }

    uint8_t avc_config[256];
    int avc_config_len = 0;
    if (rtmp_build_avc_config_record(sps_data, sps_len, pps_data, pps_len,
                                      avc_config, &avc_config_len) < 0) {
        return -1;
    }

    /* Send as AVCPacketType=0 (sequence header) */
    return rtmp_send_video_tag(fd, avc_config, avc_config_len, timestamp_ms, 0);
}

/* Send video NAL units (AVCPacketType=1) - parses concatenated NALs, strips start codes,
 * prefixes each with 4-byte length, sends as single RTMP message */
static int rtmp_send_video_nalu(int fd, const uint8_t *data, int len,
                                 uint32_t timestamp_ms)
{
    /* Collect NAL units, skip SPS/PPS (types 7,8) */
    typedef struct {
        const uint8_t *data;
        int len;
    } nal_unit_t;

    nal_unit_t nalus[16];
    int nalu_count = 0;

    int offset = 0;
    while (offset < len - 4 && nalu_count < 16) {
        int start = -1;
        for (int i = offset; i + 3 < len; i++) {
            if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
                start = i; break;
            }
            if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01) {
                start = i; break;
            }
        }
        if (start < 0) break;

        int next = -1;
        for (int i = start + 4; i + 3 < len; i++) {
            if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x00 && data[i+3] == 0x01) {
                next = i; break;
            }
            if (data[i] == 0x00 && data[i+1] == 0x00 && data[i+2] == 0x01) {
                next = i; break;
            }
        }
        int nal_end = (next < 0) ? len : next;

        int nal_code_len = 4;
        if (data[start+2] == 0x01) nal_code_len = 3;

        const uint8_t *nal = data + start + nal_code_len;
        int nal_size = nal_end - start - nal_code_len;

        if (nal_size > 0) {
            uint8_t nal_type = nal[0] & 0x1F;
            if (nal_type != 7 && nal_type != 8) { /* Skip SPS/PPS */
                nalus[nalu_count].data = nal;
                nalus[nalu_count].len = nal_size;
                nalu_count++;
            }
        }

        offset = nal_end;
    }

    if (nalu_count == 0) {
        ESP_LOGW(TAG, "RTMP: no frame NALs (AUD/IDR) found");
        return -1;
    }

    /* Calculate total length: 4-byte length prefix per NAL */
    int total_len = 0;
    for (int i = 0; i < nalu_count; i++) {
        total_len += 4 + nalus[i].len;
    }

    /* Build FLV video tag with ALL NALs concatenated (4-byte length + NAL data) */
    uint8_t flv_tag[11 + RTMP_CHUNK_SIZE];
    int hlen = 0;

    flv_tag[hlen++] = 0x09; /* tag type: video */
    rtmp_put_be24(&flv_tag[hlen], 5 + total_len); hlen += 3; /* data size */
    rtmp_put_be24(&flv_tag[hlen], timestamp_ms); hlen += 3; /* timestamp */
    flv_tag[hlen++] = 0; /* timestamp extended */
    rtmp_put_be24(&flv_tag[hlen], 0); hlen += 3; /* stream ID = 0 */

    /* FLV video body: 5 bytes with AVCPacketType=1 */
    flv_tag[hlen++] = 0x17; /* keyframe + AVC */
    flv_tag[hlen++] = 0x01; /* AVCPacketType = 1 (NALU) */
    flv_tag[hlen++] = 0x00; /* composition time */
    flv_tag[hlen++] = 0x00;
    flv_tag[hlen++] = 0x00;

    /* Concatenate all NALs with 4-byte big-endian length prefix */
    int copy_len = 0;
    for (int i = 0; i < nalu_count; i++) {
        flv_tag[hlen + copy_len++] = (nalus[i].len >> 24) & 0xFF;
        flv_tag[hlen + copy_len++] = (nalus[i].len >> 16) & 0xFF;
        flv_tag[hlen + copy_len++] = (nalus[i].len >> 8) & 0xFF;
        flv_tag[hlen + copy_len++] = nalus[i].len & 0xFF;
        memcpy(flv_tag + hlen + copy_len, nalus[i].data, nalus[i].len);
        copy_len += nalus[i].len;
        if (copy_len >= (int)(sizeof(flv_tag) - hlen - 4)) break;
    }

    /* Total body = FLV tag header (11) + FLV video body (5) + NAL data */
    return rtmp_write_chunk(fd, 0, RTMP_CHUNK_VIDEO, timestamp_ms, flv_tag, 11 + 5 + copy_len);
}

int network_tx_rtmp_send(const uint8_t *data, int len, uint32_t timestamp_ms)
{
    if (!s_rtmp.connected || !s_rtmp.stream_published) {
        return -1;
    }

    if (s_rtmp.mutex) {
        xSemaphoreTake(s_rtmp.mutex, portMAX_DELAY);
    }

    /* Update timestamp */
    if (timestamp_ms == 0) {
        timestamp_ms = s_rtmp.timestamp;
        s_rtmp.timestamp += 33; /* ~30fps */
    }

    /* Send onMetaData before first video frame (ffmpeg does this) */
    if (!s_rtmp.metadata_sent) {
        rtmp_send_on_metadata(s_rtmp.tcp_fd);
        s_rtmp.metadata_sent = true;
    }

    int ret = 0;
    if (!s_rtmp.avc_config_sent) {
        /* First frame: send SPS/PPS as sequence header (AVCPacketType=0) */
        ret = rtmp_send_video_seq_header(s_rtmp.tcp_fd, data, len, timestamp_ms);
        if (ret >= 0) {
            s_rtmp.avc_config_sent = true;
            /* Then send frame NALUs (AUD+IDR) as AVCPacketType=1 */
            ret = rtmp_send_video_nalu(s_rtmp.tcp_fd, data, len, timestamp_ms);
        }
    } else {
        /* Subsequent frames: send frame NALUs as AVCPacketType=1 */
        ret = rtmp_send_video_nalu(s_rtmp.tcp_fd, data, len, timestamp_ms);
    }

    /* Debug: log first video frame wire format for verification */
    static bool first_video_logged = false;
    if (!first_video_logged && s_rtmp.avc_config_sent && ret >= 0) {
        first_video_logged = true;
        ESP_LOGI(TAG, "RTMP: First video frame sent successfully (timestamp=%u ms)", timestamp_ms);
    }

    if (ret < 0) {
        /* Send failed - mark connection as dead so main loop retries with
           next stream ID */
        ESP_LOGW(TAG, "RTMP: send failed, marking connection as dead");
        s_rtmp.connected = false;
        s_rtmp.stream_published = false;
        if (s_rtmp.tcp_fd >= 0) {
            close(s_rtmp.tcp_fd);
            s_rtmp.tcp_fd = -1;
        }
    }

    if (s_rtmp.mutex) {
        xSemaphoreGive(s_rtmp.mutex);
    }

    return ret;
}
