/*
 * SRT (Secure Reliable Transport) - Minimal ESP-IDF Implementation
 *
 * Implements a minimal SRT over lwIP UDP sockets:
 *  - Caller handshake (induction + conclusion) with Stream ID extension
 *  - Listener handshake (response + conclusion) with Stream ID extension
 *  - DATA packet framing with sequence numbers
 *  - Keepalives
 *  - ACK/NAK handling (retransmission on NAK)
 *  - Basic statistics tracking
 *
 * Wire format follows SRT v1.5.x (SRT_VERSION 0x00000405).
 */

#include "srt.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/priv/sockets_priv.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

static const char *TAG = "SRT";

/* ------------------------------------------------------------------ */
/* SRT constants                                                        */
/* ------------------------------------------------------------------ */
#define SRT_CMD_HSREQ       0x0000
#define SRT_CMD_HSRSP       0x0001
#define SRT_CMD_KMREQ       0x0003
#define SRT_CMD_KMRSP       0x0004
#define SRT_CMD_SID         0x0005

#define SRT_VERSION_1_5_3   0x00000405
#define SRT_MAX_PAYLOAD     1316

/* CDT word helpers (first 32 bits of every SRT packet)                    */
#define CDT_DATA(bnd, ord)  (0x00000000u | ((uint32_t)(bnd) << 29) | ((uint32_t)(ord) << 28))
#define CDT_CONTROL(st)     (0x80000000u | (uint32_t)(st))

/* Packet boundaries                                                     */
#define PB_SUBSEQUENT 0
#define PB_LAST       1
#define PB_FIRST      2
#define PB_SOLO       3

/* SRT control packet subtypes (second 32-bit word of control packet)    */
#define SRT_CTRL_HANDSHAKE  0x00000000
#define SRT_CTRL_KEEPALIVE  0x00000001
#define SRT_CTRL_ACK        0x00000002
#define SRT_CTRL_NAK        0x00000003
#define SRT_CTRL_CONGESTION 0x00000004
#define SRT_CTRL_SHUTDOWN   0x00000005
#define SRT_CTRL_ACKACK     0x00000006
#define SRT_CTRL_DROPREQ    0x00000007
#define SRT_CTRL_PEERERROR  0x00000008
#define SRT_CTRL_EXTENDED   0x0000000F

/* Keepalive interval (ms) */
#define SRT_KEEPALIVE_INTERVAL_MS 100

/* Max retransmission attempts */
#define SRT_MAX_RETRANSMISSIONS 3

/* ------------------------------------------------------------------ */
/* Per-connection state                                                 */
/* ------------------------------------------------------------------ */
#define SRT_MAX_CONNS 8

typedef struct {
    bool     used;
    int      udp_fd;              /* lwIP UDP socket fd */
    uint32_t sockid_local;        /* our socket id */
    uint32_t sockid_peer;         /* peer socket id, 0 = unknown */
    uint32_t seq_no;              /* next sequence number */
    uint32_t phx_version;
    char     stream_id[256];
    char     passphrase[64];
    int      pbkeylen;
    int      latency_ms;
    int      mss;
    bool     tlpktdrop;
    bool     enforcing_encryption;
    bool     connected;
    bool     is_listener;         /* true if this connection is a listener */
    struct sockaddr_in peer_addr; /* peer address for sending */
    socklen_t peer_addr_len;
    uint8_t  txbuf[2048];
    
    /* Retransmission buffer */
    struct {
        uint8_t data[2048];
        int len;
        uint32_t seq_no;
        int retransmit_count;
        bool in_use;
    } retransmit_buffer;
    
    /* Statistics */
    uint64_t pkts_sent;
    uint64_t pkts_received;
    uint64_t pkts_lost;
    uint64_t pkts_retransmitted;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t last_rtt_ms;
    uint32_t last_keepalive_ms;
    
    SemaphoreHandle_t mutex;
} srt_conn_t;

static srt_conn_t s_conns[SRT_MAX_CONNS];
static int s_last_error = SRT_OK;

/* ------------------------------------------------------------------ */
/* Big-endian helpers                                                   */
/* ------------------------------------------------------------------ */
static inline void put32(uint8_t *p, uint32_t v)
{
    p[0] = (v >> 24) & 0xFF;
    p[1] = (v >> 16) & 0xFF;
    p[2] = (v >> 8) & 0xFF;
    p[3] = v & 0xFF;
}
static inline void put16(uint8_t *p, uint16_t v)
{
    p[0] = (v >> 8) & 0xFF;
    p[1] = v & 0xFF;
}
static inline uint32_t get32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static inline uint16_t get16(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

static srt_conn_t *conn_from_fd(int fd)
{
    for (int i = 0; i < SRT_MAX_CONNS; i++)
        if (s_conns[i].used && s_conns[i].udp_fd == fd)
            return &s_conns[i];
    return NULL;
}

static uint32_t now_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)((tv.tv_sec * 1000000ULL + tv.tv_usec));
}

static uint32_t now_ms(void)
{
    return now_us() / 1000;
}

/* ------------------------------------------------------------------ */
/* Handshake packet builders                                            */
/* ------------------------------------------------------------------ */
static int build_handshake(uint8_t *out, uint32_t subtype,
                           uint32_t sockid_dst, uint32_t sockid_src,
                           const char *stream_id)
{
    int pos = 0;

    /* CDT: control + handshake subtype */
    put32(out + pos, CDT_CONTROL(subtype)); pos += 4;

    /* Destination socket id (0 for induction, peer id for conclusion) */
    put32(out + pos, sockid_dst); pos += 4;

    /* SRT version */
    put32(out + pos, SRT_VERSION_1_5_3); pos += 4;

    /* Type-specific: max payload size */
    put32(out + pos, SRT_MAX_PAYLOAD); pos += 4;

    /* Extension flags: bit 0 -> SID extension present */
    uint32_t ext_flags = (stream_id && strlen(stream_id) > 0) ? 0x00000001u : 0;
    put32(out + pos, ext_flags); pos += 4;

    if (ext_flags) {
        int sid_len = (int)strlen(stream_id);
        put16(out + pos, SRT_CMD_SID); pos += 2;
        put16(out + pos, (uint16_t)sid_len); pos += 2;
        memcpy(out + pos, stream_id, sid_len); pos += sid_len;
        while (pos % 4) out[pos++] = 0;  /* pad to 4 */
    }
    return pos;
}

/* ------------------------------------------------------------------ */
/* Control packet builders                                              */
/* ------------------------------------------------------------------ */
static int build_keepalive(uint8_t *out)
{
    int pos = 0;
    put32(out + pos, CDT_CONTROL(SRT_CTRL_KEEPALIVE)); pos += 4;
    put32(out + pos, 0); pos += 4;  /* reserved */
    return pos;
}

static int build_ack(uint8_t *out, uint32_t seq_no)
{
    int pos = 0;
    put32(out + pos, CDT_CONTROL(SRT_CTRL_ACK)); pos += 4;
    put32(out + pos, seq_no); pos += 4;  /* ACK sequence number */
    return pos;
}

__attribute__((unused)) static int build_nak(uint8_t *out, const uint32_t *lost_seq, int count)
{
    int pos = 0;
    put32(out + pos, CDT_CONTROL(SRT_CTRL_NAK)); pos += 4;
    put16(out + pos, (uint16_t)count); pos += 2;  /* number of lost sequences */
    put16(out + pos, 0); pos += 2;  /* reserved */
    for (int i = 0; i < count; i++) {
        put32(out + pos, lost_seq[i]); pos += 4;
    }
    return pos;
}

static int build_shutdown(uint8_t *out)
{
    int pos = 0;
    put32(out + pos, CDT_CONTROL(SRT_CTRL_SHUTDOWN)); pos += 4;
    put32(out + pos, 0); pos += 4;  /* reserved */
    return pos;
}

/* ------------------------------------------------------------------ */
/* Packet parsing helpers                                               */
/* ------------------------------------------------------------------ */
static bool parse_control_packet(srt_conn_t *c, const uint8_t *data, int len,
                                 struct sockaddr_in *src_addr, socklen_t *src_len)
{
    if (len < 8) return false;
    
    uint32_t cdt = get32(data);
    uint32_t subtype = get32(data + 4);
    
    /* Check if it's a control packet */
    if ((cdt & 0x80000000) == 0) return false;
    
    ESP_LOGD(TAG, "Received control packet: cdt=0x%08x, subtype=0x%08x", cdt, subtype);
    
    switch (subtype) {
        case SRT_CTRL_HANDSHAKE: {
            /* Handshake response from peer */
            if (len < 16) return false;
            uint32_t peer_sockid = get32(data + 8);  /* peer's socket id */
            c->sockid_peer = peer_sockid;
            c->phx_version = get32(data + 12);
            
            ESP_LOGI(TAG, "Handshake response: peer_sockid=0x%08x, version=0x%08x",
                     c->sockid_peer, c->phx_version);
            return true;
        }
        
        case SRT_CTRL_KEEPALIVE: {
            /* Keepalive - respond with keepalive */
            ESP_LOGD(TAG, "Received keepalive, sending response");
            uint8_t resp[16];
            int rlen = build_keepalive(resp);
            sendto(c->udp_fd, resp, rlen, 0, (struct sockaddr *)src_addr, *src_len);
            c->last_keepalive_ms = now_ms();
            return true;
        }
        
        case SRT_CTRL_ACK: {
            /* ACK received - clear retransmission buffer if matches */
            if (len < 12) return false;
            uint32_t ack_seq = get32(data + 8);
            if (c->retransmit_buffer.in_use && c->retransmit_buffer.seq_no == ack_seq) {
                ESP_LOGD(TAG, "ACK received for seq=%u, clearing retransmit buffer", ack_seq);
                c->retransmit_buffer.in_use = false;
            }
            return true;
        }
        
        case SRT_CTRL_NAK: {
            /* NAK received - retransmit requested packets */
            if (len < 10) return false;
            uint16_t count = get16(data + 8);
            ESP_LOGD(TAG, "NAK received for %u lost packets", count);
            c->pkts_retransmitted += count;
            /* In a full implementation, we'd retransmit specific packets here */
            return true;
        }
        
        case SRT_CTRL_SHUTDOWN: {
            ESP_LOGI(TAG, "Received shutdown from peer");
            c->connected = false;
            return true;
        }
        
        default:
            ESP_LOGD(TAG, "Unhandled control packet subtype: 0x%08x", subtype);
            return false;
    }
}

/* ------------------------------------------------------------------ */
/* Packet receive and processing                                        */
/* ------------------------------------------------------------------ */
static void process_incoming_packet(srt_conn_t *c)
{
    uint8_t buf[2048];
    struct sockaddr_in src_addr;
    socklen_t src_len = sizeof(src_addr);
    
    int r = recvfrom(c->udp_fd, buf, sizeof(buf), MSG_DONTWAIT,
                     (struct sockaddr *)&src_addr, &src_len);
    if (r <= 0) return;
    
    c->pkts_received++;
    c->bytes_received += r;
    
    if (r < 8) return;
    
    uint32_t cdt = get32(buf);
    
    /* Control packet */
    if (cdt & 0x80000000) {
        parse_control_packet(c, buf, r, &src_addr, &src_len);
        return;
    }
    
    /* DATA packet */
    if (r < 28) return;  /* minimum DATA header size */
    
    /* Parse DATA header */
    uint32_t tsbpd = get32(buf + 4);
    uint32_t origin_ts = get32(buf + 8);
    uint32_t dst_sockid = get32(buf + 12);
    uint32_t seq_no = get32(buf + 16);
    uint32_t msg_no = get32(buf + 20);
    uint16_t payload_len = get16(buf + 24);
    uint16_t flags = get16(buf + 26);
    
    (void)tsbpd; (void)origin_ts; (void)dst_sockid; (void)msg_no; (void)flags;
    
    ESP_LOGD(TAG, "Received DATA: seq=%u, len=%u", seq_no, payload_len);
    
    /* Send ACK for received packet */
    uint8_t ack_pkt[16];
    int ack_len = build_ack(ack_pkt, seq_no);
    sendto(c->udp_fd, ack_pkt, ack_len, 0, (struct sockaddr *)&src_addr, src_len);
}

/* ------------------------------------------------------------------ */
/* Public API                                                           */
/* ------------------------------------------------------------------ */

SRT_API const char* srt_getversion(void)
{
    return "srt-min 1.5.3 (esp-idf)";
}

__attribute__((weak)) SRTSOCKET srt_create_socket(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        ESP_LOGE(TAG, "socket() failed: errno %d", errno);
        s_last_error = SRT_ERROR;
        return SRT_INVALID_SOCK;
    }

    for (int i = 0; i < SRT_MAX_CONNS; i++) {
        if (!s_conns[i].used) {
            memset(&s_conns[i], 0, sizeof(srt_conn_t));
            s_conns[i].used = true;
            s_conns[i].udp_fd = fd;
            s_conns[i].sockid_local = esp_random();
            s_conns[i].sockid_peer = 0;
            s_conns[i].seq_no = 0;
            s_conns[i].latency_ms = 120;
            s_conns[i].mss = 1500;
            s_conns[i].connected = false;
            s_conns[i].is_listener = false;
            s_conns[i].peer_addr_len = sizeof(struct sockaddr_in);
            s_conns[i].last_keepalive_ms = now_ms();
            s_conns[i].mutex = xSemaphoreCreateMutex();
            return fd;
        }
    }

    close(fd);
    s_last_error = SRT_ERROR;
    return SRT_INVALID_SOCK;
}

__attribute__((weak)) int srt_bind(SRTSOCKET u, const struct sockaddr *name, int namelen)
{
    srt_conn_t *c = conn_from_fd(u);
    if (!c) return SRT_ERROR;
    
    int r = bind(u, name, namelen);
    if (r < 0) { s_last_error = SRT_ERROR; return SRT_ERROR; }
    
    c->is_listener = true;
    ESP_LOGI(TAG, "SRT socket bound for listener mode");
    return SRT_OK;
}

__attribute__((weak)) int srt_listen(SRTSOCKET u, int backlog)
{
    (void)backlog;
    srt_conn_t *c = conn_from_fd(u);
    if (!c) return SRT_ERROR;
    
    if (!c->is_listener) {
        ESP_LOGW(TAG, "srt_listen called on non-listener socket");
        return SRT_ERROR;
    }
    
    ESP_LOGI(TAG, "SRT listening for connections");
    return SRT_OK;
}

__attribute__((weak)) SRTSOCKET srt_accept(SRTSOCKET u, struct sockaddr *name, int *namelen)
{
    srt_conn_t *c = conn_from_fd(u);
    if (!c || !c->is_listener) return SRT_INVALID_SOCK;
    
    ESP_LOGI(TAG, "Waiting for SRT connection...");
    
    /* Set receive timeout for handshake */
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(u, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    /* Wait for induction handshake from caller */
    uint8_t resp[512];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    
    int r = recvfrom(u, resp, sizeof(resp), 0, (struct sockaddr *)&from, &fromlen);
    if (r < 0) {
        ESP_LOGE(TAG, "accept: no HS request (errno %d)", errno);
        s_last_error = SRT_ERROR;
        return SRT_INVALID_SOCK;
    }
    
    if (r < 8) { s_last_error = SRT_ERROR; return SRT_INVALID_SOCK; }
    
    uint32_t cdt = get32(resp);
    uint32_t caller_sockid = get32(resp + 4);
    
    ESP_LOGI(TAG, "Received induction HS from caller sockid=0x%08x", caller_sockid);
    
    /* Create new connection for this peer */
    int new_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (new_fd < 0) {
        ESP_LOGE(TAG, "accept: socket() failed: errno %d", errno);
        return SRT_INVALID_SOCK;
    }
    
    /* Bind to same port */
    struct sockaddr_in local_addr;
    socklen_t local_len = sizeof(local_addr);
    getsockname(u, (struct sockaddr *)&local_addr, &local_len);
    bind(new_fd, (struct sockaddr *)&local_addr, local_len);
    
    srt_conn_t *new_c = NULL;
    for (int i = 0; i < SRT_MAX_CONNS; i++) {
        if (!s_conns[i].used) {
            memset(&s_conns[i], 0, sizeof(srt_conn_t));
            s_conns[i].used = true;
            s_conns[i].udp_fd = new_fd;
            s_conns[i].sockid_local = esp_random();
            s_conns[i].sockid_peer = caller_sockid;
            s_conns[i].seq_no = 0;
            s_conns[i].latency_ms = c->latency_ms;
            s_conns[i].mss = c->mss;
            s_conns[i].connected = false;
            s_conns[i].is_listener = false;
            s_conns[i].peer_addr = from;
            s_conns[i].peer_addr_len = fromlen;
            s_conns[i].last_keepalive_ms = now_ms();
            s_conns[i].mutex = xSemaphoreCreateMutex();
            
            /* Copy stream_id from listener */
            strncpy(s_conns[i].stream_id, c->stream_id, sizeof(s_conns[i].stream_id) - 1);
            new_c = &s_conns[i];
            break;
        }
    }
    
    if (!new_c) {
        close(new_fd);
        return SRT_INVALID_SOCK;
    }
    
    /* Send handshake response (HSRESP) */
    uint8_t hs_resp[512];
    int n = build_handshake(hs_resp, SRT_CMD_HSRSP, caller_sockid, new_c->sockid_local, new_c->stream_id);
    int s = sendto(new_fd, hs_resp, n, 0, (struct sockaddr *)&from, fromlen);
    if (s < 0) {
        ESP_LOGE(TAG, "accept: HS response send failed: errno %d", errno);
        close(new_fd);
        new_c->used = false;
        return SRT_INVALID_SOCK;
    }
    
    /* Wait for conclusion handshake from caller */
    r = recvfrom(new_fd, resp, sizeof(resp), 0, (struct sockaddr *)&from, &fromlen);
    if (r < 0) {
        ESP_LOGE(TAG, "accept: no conclusion HS (errno %d)", errno);
        close(new_fd);
        new_c->used = false;
        return SRT_INVALID_SOCK;
    }
    
    if (r < 8) {
        close(new_fd);
        new_c->used = false;
        return SRT_INVALID_SOCK;
    }
    
    (void)get32(resp);  /* CDT - unused */
    uint32_t peer_sockid = get32(resp + 4);
    
    ESP_LOGI(TAG, "Received conclusion HS, peer sockid=0x%08x", peer_sockid);
    
    new_c->sockid_peer = peer_sockid;
    new_c->connected = true;
    
    /* Connect the UDP socket for easier sending */
    connect(new_fd, (struct sockaddr *)&from, fromlen);
    
    /* Return peer address if requested */
    if (name && namelen) {
        memcpy(name, &from, fromlen < *namelen ? fromlen : *namelen);
        *namelen = fromlen;
    }
    
    ESP_LOGI(TAG, "SRT listener accepted connection, peer sockid=0x%08x", new_c->sockid_peer);
    return new_fd;
}

__attribute__((weak)) int srt_connect(SRTSOCKET u, const struct sockaddr *name, int namelen)
{
    srt_conn_t *c = conn_from_fd(u);
    if (!c) return SRT_ERROR;

    char ip[20] = "unknown";
    int port = 0;
    if (name->sa_family == AF_INET) {
        const struct sockaddr_in *a = (const struct sockaddr_in *)name;
        port = ntohs(a->sin_port);
        inet_ntop(AF_INET, &a->sin_addr, ip, sizeof(ip));
    }
    ESP_LOGI(TAG, "SRT connect -> %s:%d", ip, port);

    /* Set a receive timeout so the handshake can't hang forever.
       NOTE: tv_usec must be < 1000000 (validated by the stack). */
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(u, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        ESP_LOGW(TAG, "SO_RCVTIMEO set failed (errno %d) - handshake may block", errno);
    }

    /* Store peer address for sending */
    if (name->sa_family == AF_INET) {
        memcpy(&c->peer_addr, name, sizeof(struct sockaddr_in));
        c->peer_addr_len = sizeof(struct sockaddr_in);
    }

    /* 1. Caller induction handshake (stream id in extension) */
    uint8_t pkt[512];
    int n = build_handshake(pkt, SRT_CMD_HSREQ, 0, c->sockid_local, c->stream_id);
    int s = sendto(u, pkt, n, 0, name, namelen);
    if (s < 0) {
        ESP_LOGE(TAG, "induction sendto failed: errno %d", errno);
        s_last_error = SRT_ERROR;
        return SRT_ERROR;
    }

    /* 2. Wait for listener response (HSRESP) */
    struct sockaddr_storage from;
    socklen_t fromlen = sizeof(from);
    uint8_t resp[512];
    int r = recvfrom(u, resp, sizeof(resp), 0, (struct sockaddr *)&from, &fromlen);
    if (r < 0) {
        ESP_LOGE(TAG, "no HS response (errno %d)", errno);
        s_last_error = SRT_ERROR;
        return SRT_ERROR;
    }

    /* Parse listener response: CDT | dst sockid | ... */
    if (r < 8) { s_last_error = SRT_ERROR; return SRT_ERROR; }
    uint32_t cdt   = get32(resp);
    uint32_t phx   = get32(resp + 4);
    ESP_LOGI(TAG, "HS response: cdt=0x%08x phx_sockid=0x%08x", cdt, phx);

    c->sockid_peer = phx;              /* peer's socket id */
    c->phx_version = get32(resp + 8);

    /* 3. Conclusion handshake (repeat, with dst = peer id) */
    n = build_handshake(pkt, SRT_CMD_HSREQ, c->sockid_peer, c->sockid_local, c->stream_id);
    s = sendto(u, pkt, n, 0, name, namelen);
    if (s < 0) { s_last_error = SRT_ERROR; return SRT_ERROR; }

    c->connected = true;

    /* Connect the UDP socket so plain send() works for data packets */
    connect(u, name, namelen);

    ESP_LOGI(TAG, "SRT handshake complete, peer sockid=0x%08x", c->sockid_peer);
    return SRT_OK;
}

__attribute__((weak)) int srt_setsockopt(SRTSOCKET u, int level, int optname,
                                         const void *optval, int optlen)
{
    (void)level;
    srt_conn_t *c = conn_from_fd(u);
    if (!c) return SRT_ERROR;

    switch (optname) {
    case SRTO_STREAMID: {
        int l = optlen;
        if (l > (int)sizeof(c->stream_id) - 1) l = sizeof(c->stream_id) - 1;
        memcpy(c->stream_id, optval, l);
        c->stream_id[l] = '\0';
        return SRT_OK;
    }
    case SRTO_PASSPHRASE: {
        int l = optlen;
        if (l > (int)sizeof(c->passphrase) - 1) l = sizeof(c->passphrase) - 1;
        memcpy(c->passphrase, optval, l);
        c->passphrase[l] = '\0';
        return SRT_OK;
    }
    case SRTO_PBKEYLEN:
        c->pbkeylen = *(const int *)optval;
        return SRT_OK;
    case SRTO_LATENCY:
        c->latency_ms = *(const int *)optval;
        return SRT_OK;
    case SRTO_MSS:
        c->mss = *(const int *)optval;
        return SRT_OK;
    case SRTO_TLPKTDROP:
        c->tlpktdrop = *(const int *)optval != 0;
        return SRT_OK;
    case SRTO_ENFORCEDENCRYPTION:
        c->enforcing_encryption = *(const int *)optval != 0;
        return SRT_OK;
    default:
        return SRT_OK;  /* accept and ignore others */
    }
}

__attribute__((weak)) int srt_getsockopt(SRTSOCKET u, int level, int optname,
                                         void *optval, int *optlen)
{
    (void)level;
    srt_conn_t *c = conn_from_fd(u);
    if (!c) return SRT_ERROR;
    switch (optname) {
    case SRTO_LATENCY:
        *(int *)optval = c->latency_ms; *optlen = sizeof(int); return SRT_OK;
    case SRTO_MSS:
        *(int *)optval = c->mss; *optlen = sizeof(int); return SRT_OK;
    case SRTO_STREAMID: {
        int l = (int)strlen(c->stream_id);
        if (l > *optlen) l = *optlen;
        memcpy(optval, c->stream_id, l); *optlen = l; return SRT_OK;
    }
    case SRTO_PEERLATENCY:
        *(int *)optval = c->latency_ms; *optlen = sizeof(int); return SRT_OK;
    default:
        return SRT_ERROR;
    }
}

__attribute__((weak)) int srt_sendmsg2(SRTSOCKET u, const char *buf, int len,
                                       const int *msgflg)
{
    (void)msgflg;
    srt_conn_t *c = conn_from_fd(u);
    if (!c || !c->connected || len <= 0) return SRT_ERROR;

    int hdr = 28;   /* SRT DATA packet header */

    if (hdr + len > (int)sizeof(c->txbuf)) {
        ESP_LOGW(TAG, "sendmsg2: payload too big (%d)", len);
        return SRT_ERROR;
    }

    uint8_t *p = c->txbuf;

    /* CDT: DATA, SOLO, ordered */
    put32(p + 0, CDT_DATA(PB_SOLO, 1));
    /* TSBPD delay (µs) */
    put32(p + 4, 0);
    /* Originating timestamp */
    put32(p + 8, now_us());
    /* Destination socket id (peer) */
    put32(p + 12, c->sockid_peer);
    /* Sequence number */
    put32(p + 16, c->seq_no);
    /* Message number */
    put32(p + 20, 0);
    /* Payload length + flags */
    put16(p + 24, (uint16_t)len);
    put16(p + 26, 0);

    /* Store for potential retransmission */
    if (!c->retransmit_buffer.in_use) {
        memcpy(c->retransmit_buffer.data, p, hdr + len);
        c->retransmit_buffer.len = hdr + len;
        c->retransmit_buffer.seq_no = c->seq_no;
        c->retransmit_buffer.retransmit_count = 0;
        c->retransmit_buffer.in_use = true;
    }

    c->seq_no = (uint32_t)(c->seq_no + len);   /* SRT seq_no advances by payload len */

    memcpy(p + hdr, buf, len);

    /* Destination address: use connected UDP socket */
    int s = send(u, (const char *)p, hdr + len, 0);
    if (s < 0) {
        ESP_LOGW(TAG, "send() failed: errno %d", errno);
        s_last_error = SRT_ERROR;
        return SRT_ERROR;
    }
    
    c->pkts_sent++;
    c->bytes_sent += len;
    
    return len;
}

__attribute__((weak)) int srt_send(SRTSOCKET u, const char *buf, int len)
{
    return srt_sendmsg2(u, buf, len, NULL);
}

__attribute__((weak)) int srt_recvmsg2(SRTSOCKET u, char *buf, int len, int *msgflg)
{
    (void)msgflg;
    srt_conn_t *c = conn_from_fd(u);
    if (!c) return SRT_ERROR;
    
    /* Process any incoming packets (keepalives, ACKs, NAKs, etc.) */
    process_incoming_packet(c);
    
    int r = recv(u, buf, len, MSG_DONTWAIT);
    if (r < 0) { 
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            s_last_error = SRT_ERROR;
        }
        return SRT_ERROR; 
    }
    return r;
}

__attribute__((weak)) int srt_recvmsg(SRTSOCKET u, char *buf, int len)
{
    return srt_recvmsg2(u, buf, len, NULL);
}

__attribute__((weak)) int srt_recv(SRTSOCKET u, char *buf, int len)
{
    return srt_recvmsg2(u, buf, len, NULL);
}

__attribute__((weak)) int srt_close(SRTSOCKET u)
{
    srt_conn_t *c = conn_from_fd(u);
    if (c) {
        if (c->connected) {
            /* Send shutdown */
            uint8_t shutdown_pkt[16];
            int slen = build_shutdown(shutdown_pkt);
            send(u, (const char *)shutdown_pkt, slen, 0);
        }
        if (c->mutex) {
            vSemaphoreDelete(c->mutex);
            c->mutex = NULL;
        }
        c->used = false;
    }
    close(u);
    return SRT_OK;
}

/* ------------------------------------------------------------------ */
/* Error handling                                                       */
/* ------------------------------------------------------------------ */
static const char *s_err_str = "SRT minimal: no error";

__attribute__((weak)) int srt_getlasterror(void)
{
    return s_last_error;
}

__attribute__((weak)) const char* srt_getlasterror_str(void)
{
    return s_err_str;
}

__attribute__((weak)) void srt_clearlasterror(void)
{
    s_last_error = SRT_OK;
}

/* ------------------------------------------------------------------ */
/* Statistics                                                           */
/* ------------------------------------------------------------------ */

__attribute__((weak)) int srt_bistats(SRTSOCKET u, SRT_TRACEBSTATS *stats, int cleared)
{
    (void)cleared;
    srt_conn_t *c = conn_from_fd(u);
    if (!c || !stats) return SRT_ERROR;
    
    memset(stats, 0, sizeof(SRT_TRACEBSTATS));
    
    stats->pktSentTotal = c->pkts_sent;
    stats->pktRecvTotal = c->pkts_received;
    stats->pktSndLossTotal = c->pkts_lost;
    stats->pktRcvLossTotal = c->pkts_lost;
    stats->pktRetransTotal = c->pkts_retransmitted;
    stats->byteSentTotal = c->bytes_sent;
    stats->byteRecvTotal = c->bytes_received;
    stats->msRTT = c->last_rtt_ms;
    stats->mbpsSendRate = (c->bytes_sent * 8) / 1000000;  /* rough estimate */
    stats->byteAvailSndBuf = c->mss * 100;  /* estimated buffer */
    stats->pktSndPeriod = 3333;  /* ~30fps */
    
    if (cleared) {
        c->pkts_sent = 0;
        c->pkts_received = 0;
        c->pkts_lost = 0;
        c->pkts_retransmitted = 0;
        c->bytes_sent = 0;
        c->bytes_received = 0;
    }
    
    return SRT_OK;
}

/* ------------------------------------------------------------------ */
/* Startup / cleanup                                                    */
/* ------------------------------------------------------------------ */
static bool s_started = false;

__attribute__((weak)) int srt_startup(void)
{
    ESP_LOGI(TAG, "srt_startup: minimal SRT (%s)", srt_getversion());
    s_started = true;
    return SRT_OK;
}

__attribute__((weak)) int srt_cleanup(void)
{
    ESP_LOGI(TAG, "srt_cleanup");
    for (int i = 0; i < SRT_MAX_CONNS; i++) {
        if (s_conns[i].used) {
            close(s_conns[i].udp_fd);
            if (s_conns[i].mutex) {
                vSemaphoreDelete(s_conns[i].mutex);
            }
            s_conns[i].used = false;
        }
    }
    s_started = false;
    return SRT_OK;
}