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
 * Data sent as FLV video tags (H.264 NAL units extracted from TS packets).
 */

#include "network_tx_rtmp.h"
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
#define RTMP_CHUNK_VIDEO 8

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
} s_rtmp = {0};

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

    ESP_LOGI(TAG, "RTMP handshake complete (S0+S1+S2 received)");
    return 0;
}

/* ------------------------------------------------------------------ */
/* RTMP Chunk Writing                                                 */
/* ------------------------------------------------------------------ */

static int rtmp_write_chunk(int fd, uint8_t chunk_type, uint32_t chunk_stream_id,
                            uint32_t timestamp, const uint8_t *data, int len)
{
    uint8_t header[12];
    int hlen = 0;

    /* Chunk Type 0: 11-byte header (fmt=0, csid follows) */
    header[0] = (chunk_type << 6) | chunk_stream_id;
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

    /* Type (1 byte) */
    header[hlen++] = RTMP_CHUNK_INVOKE;

    /* Stream ID (4 bytes, little-endian) */
    header[hlen++] = chunk_stream_id & 0xFF;
    header[hlen++] = (chunk_stream_id >> 8) & 0xFF;
    header[hlen++] = (chunk_stream_id >> 16) & 0xFF;
    header[hlen++] = (chunk_stream_id >> 24) & 0xFF;

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

        /* If chunked, send timestamp extended + chunk stream id for subsequent chunks */
        if (offset < len) {
            uint8_t ext_header[4];
            ext_header[0] = (chunk_type << 6) | chunk_stream_id;
            uint32_t ext_ts = timestamp;
            if (ext_ts >= 0xFFFFFF) ext_ts = 0xFFFFFF;
            rtmp_put_be24(&ext_header[1], ext_ts);
            if (rtmp_write(fd, ext_header, 4) < 0) return -1;
        }
    }

    return 0;
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
    rtmp_put_amf_string_prop(&p, "tcUrl", s_rtmp.app);
    rtmp_put_amf_object_prop(&p, "fpad", 0.0);
    rtmp_put_amf_object_prop(&p, "capabilities", 239.0);
    rtmp_put_amf_object_prop(&p, "audioCodecs", 3191.0);
    rtmp_put_amf_object_prop(&p, "videoCodecs", 252.0);
    rtmp_put_amf_object_prop(&p, "videoFunction", 1.0);
    rtmp_put_amf_object_prop(&p, "objectEncoding", 0.0);
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
    rtmp_put_amf_string(&p, "live");

    return rtmp_write_chunk(fd, 0, RTMP_CHUNK_INVOKE, 0, buf, (int)(p - buf));
}

/* ------------------------------------------------------------------ */
/* RTMP Video Tag                                                     */
/* ------------------------------------------------------------------ */

/* FLV video tag header:
 *   1 byte:  tag type (9 = video)
 *   3 bytes: data size (big-endian)
 *   3 bytes: timestamp (big-endian)
 *   1 byte:  timestamp extended
 *   3 bytes: stream ID (always 0)
 */
static int rtmp_send_video_tag(int fd, const uint8_t *data, int len,
                               uint32_t timestamp_ms)
{
    /* FLV video tag: 11-byte header + data */
    uint8_t header[11];
    int hlen = 0;

    header[hlen++] = 0x09; /* tag type: video */
    rtmp_put_be24(&header[hlen], len); hlen += 3; /* data size */
    rtmp_put_be24(&header[hlen], timestamp_ms); hlen += 3; /* timestamp */
    header[hlen++] = 0; /* timestamp extended */
    rtmp_put_be24(&header[hlen], 0); hlen += 3; /* stream ID = 0 */

    /* Write header + first chunk */
    uint8_t combined[11 + RTMP_CHUNK_SIZE];
    memcpy(combined, header, hlen);
    int copy_len = len < (int)(sizeof(combined) - hlen) ? len : (int)(sizeof(combined) - hlen);
    memcpy(combined + hlen, data, copy_len);

    if (rtmp_write_chunk(fd, 0, RTMP_CHUNK_VIDEO, timestamp_ms, combined, hlen + copy_len) < 0) {
        return -1;
    }

    /* Write remaining data */
    int offset = copy_len;
    while (offset < len) {
        int chunk_len = len - offset;
        if (s_rtmp.chunk_size > 0 && chunk_len > (int)s_rtmp.chunk_size) {
            chunk_len = s_rtmp.chunk_size;
        }
        if (rtmp_write_chunk(fd, 1, RTMP_CHUNK_VIDEO, timestamp_ms, data + offset, chunk_len) < 0) {
            return -1;
        }
        offset += chunk_len;
    }

    return 0;
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

/* Extract H.264 NAL units from a TS packet buffer and send each as FLV video tag */
static int rtmp_send_ts_as_video(int fd, const uint8_t *ts_data, int ts_len,
                                 uint32_t timestamp_ms)
{
    int offset = 0;
    int nal_count = 0;

    while (offset < ts_len - 4) {
        int start = find_start_code(ts_data, ts_len, offset);
        if (start < 0) break;

        int next = find_start_code(ts_data, ts_len, start + 4);
        int nal_len = (next < 0) ? (ts_len - start) : (next - start);

        /* Skip the start code bytes */
        int nal_start = start;
        int nal_code_len = 4;
        if (ts_data[start] == 0x00 && ts_data[start+1] == 0x00 &&
            ts_data[start+2] == 0x00 && ts_data[start+3] == 0x01) {
            nal_code_len = 4;
        } else {
            nal_code_len = 3;
            nal_start = start;
        }

        const uint8_t *nal_data = ts_data + nal_start + nal_code_len;
        int nal_data_len = nal_len - nal_code_len;

        if (nal_data_len > 0) {
            uint8_t nal_type = nal_data[0] & 0x1F;
            /* Only send VCL NAL units (1-5 are IDR/non-IDR slices, 7=SPS, 8=PPS) */
            if (nal_type <= 5 || nal_type == 7 || nal_type == 8) {
                if (rtmp_send_video_tag(fd, nal_data, nal_data_len,
                                        timestamp_ms) < 0) {
                    return -1;
                }
                nal_count++;
            }
        }

        offset = start + nal_len;
    }

    return nal_count;
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

    ESP_LOGI(TAG, "RTMP: connecting to %s:%d app=%s stream=%s",
             s_rtmp.server, s_rtmp.port, s_rtmp.app, s_rtmp.stream);

    if (connect(fd, (struct sockaddr *)&dst, sizeof(dst)) < 0) {
        if (errno != EINPROGRESS) {
            ESP_LOGE(TAG, "RTMP: connect() failed: errno %d", errno);
            close(fd);
            return ESP_FAIL;
        }
        /* Wait for connect to complete */
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        /* Simple wait - in production use select/poll */
        vTaskDelay(pdMS_TO_TICKS(1000));

        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
        if (so_error != 0) {
            ESP_LOGE(TAG, "RTMP: connect() error: %d", so_error);
            close(fd);
            return ESP_FAIL;
        }
    }

    /* RTMP handshake */
    if (rtmp_handshake(fd) < 0) {
        close(fd);
        return ESP_FAIL;
    }

    /* Connect command */
    if (rtmp_send_connect(fd) < 0) {
        ESP_LOGE(TAG, "RTMP: connect command failed");
        close(fd);
        return ESP_FAIL;
    }

    /* CreateStream */
    if (rtmp_send_create_stream(fd) < 0) {
        ESP_LOGE(TAG, "RTMP: createStream command failed");
        close(fd);
        return ESP_FAIL;
    }

    /* Publish */
    if (rtmp_send_publish(fd) < 0) {
        ESP_LOGE(TAG, "RTMP: publish command failed");
        close(fd);
        return ESP_FAIL;
    }

    s_rtmp.tcp_fd = fd;
    s_rtmp.connected = true;
    s_rtmp.stream_published = true;
    s_rtmp.timestamp = 0;
    s_rtmp.mutex = xSemaphoreCreateMutex();

    ESP_LOGI(TAG, "RTMP: connected and publishing to %s/%s",
             s_rtmp.app, s_rtmp.stream);
    return ESP_OK;
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

    int ret = rtmp_send_ts_as_video(s_rtmp.tcp_fd, data, len, timestamp_ms);

    if (s_rtmp.mutex) {
        xSemaphoreGive(s_rtmp.mutex);
    }

    return ret;
}
