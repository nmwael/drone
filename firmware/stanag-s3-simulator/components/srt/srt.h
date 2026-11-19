/*
 * SRT - Secure, Reliable, Transport
 * Minimal C API Header for ESP-IDF
 * Based on Haivision SRT (https://github.com/Haivision/srt)
 * 
 * This is a minimal header for the SRT C API.
 * Full implementation requires Haivision SRT library.
 */

#ifndef INC_SRTC_H
#define INC_SRTC_H

#include <stdint.h>
#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SRT API visibility */
#ifndef SRT_API
#define SRT_API
#endif

/* Version */
#define SRT_VERSION_MAJOR 1
#define SRT_VERSION_MINOR 5
#define SRT_VERSION_PATCH 7
#define SRT_VERSION_VALUE ((SRT_VERSION_MAJOR << 16) | (SRT_VERSION_MINOR << 8) | SRT_VERSION_PATCH)

/* Socket handle */
typedef int SRTSOCKET;
#define SRT_INVALID_SOCK (-1)

/* Error codes */
#define SRT_ERROR (-1)
#define SRT_OK 0

/* Socket options */
#define SRTO_SNDSYN  1
#define SRTO_RCVSYN  2
#define SRTO_LATENCY 3
#define SRTO_MSS     4
#define SRTO_TLPKTDROP 5
#define SRTO_PASSPHRASE 6
#define SRTO_PBKEYLEN 7
#define SRTO_ENFORCEDENCRYPTION 8
#define SRTO_STREAMID 9
#define SRTO_SENDER  10
#define SRTO_RCVBUF  11
#define SRTO_SNDBUF  12
#define SRTO_TSBPDMODE 13
#define SRTO_TSBPDDELAY 14
#define SRTO_CONNTIMEO 15
#define SRTO_LOSSMAXTTL 16
#define SRTO_RCVLATENCY 17
#define SRTO_PEERLATENCY 18
#define SRTO_MINVERSION 19

/* Connection modes */
#define SRTO_MODE_CALLER 0
#define SRTO_MODE_LISTENER 1
#define SRTO_MODE_RENDEZVOUS 2

/* Handshake types (for internal use) */
#define SRT_CMD_HSREQ 0x0000
#define SRT_CMD_HSRSP 0x0001

/* Status */
#define SRTS_UNKNOWN 0
#define SRTS_LISTENING 1
#define SRTS_CONNECTING 2
#define SRTS_CONNECTED 3
#define SRTS_BROKEN 4
#define SRTS_CLOSING 5
#define SRTS_CLOSED 6

/* Socket functions - stub implementations */
SRT_API SRTSOCKET srt_create_socket(void);
SRT_API int srt_bind(SRTSOCKET u, const struct sockaddr* name, int namelen);
SRT_API int srt_listen(SRTSOCKET u, int backlog);
SRT_API SRTSOCKET srt_accept(SRTSOCKET u, struct sockaddr* name, int* namelen);
SRT_API int srt_connect(SRTSOCKET u, const struct sockaddr* name, int namelen);
SRT_API int srt_close(SRTSOCKET u);
SRT_API int srt_setsockopt(SRTSOCKET u, int level, int optname, const void* optval, int optlen);
SRT_API int srt_getsockopt(SRTSOCKET u, int level, int optname, void* optval, int* optlen);
SRT_API int srt_sendmsg2(SRTSOCKET u, const char* buf, int len, const int* msgflg);
SRT_API int srt_recvmsg2(SRTSOCKET u, char* buf, int len, int* msgflg);
SRT_API int srt_send(SRTSOCKET u, const char* buf, int len);
SRT_API int srt_recv(SRTSOCKET u, char* buf, int len);

/* Error handling */
SRT_API int srt_getlasterror(void);
SRT_API const char* srt_getlasterror_str(void);
SRT_API void srt_clearlasterror(void);

/* Statistics */
typedef struct {
    int64_t pktSentTotal;
    int64_t pktRecvTotal;
    int64_t pktSndLossTotal;
    int64_t pktRcvLossTotal;
    int64_t pktRetransTotal;
    int64_t pktSndDropTotal;
    int64_t pktRcvDropTotal;
    int64_t pktSndFilterTotal;
    int64_t pktRcvFilterTotal;
    int64_t pktSndUndecryptTotal;
    int64_t byteSentTotal;
    int64_t byteRecvTotal;
    int64_t byteSndDropTotal;
    int64_t byteRcvDropTotal;
    int64_t byteSndUndecryptTotal;
    int64_t pktSndTotalOld;
    int64_t pktRcvTotalOld;
    int64_t pktSndLossTotalOld;
    int64_t pktRcvLossTotalOld;
    int64_t pktRetransTotalOld;
    int64_t pktSndDropTotalOld;
    int64_t pktRcvDropTotalOld;
    int64_t pktSndFilterTotalOld;
    int64_t pktRcvFilterTotalOld;
    int64_t pktSndUndecryptTotalOld;
    int64_t byteSentTotalOld;
    int64_t byteRecvTotalOld;
    int64_t byteSndDropTotalOld;
    int64_t byteRcvDropTotalOld;
    int64_t byteSndUndecryptTotalOld;
    int msRTT;
    int mbpsSendRate;
    int mbpsRecvRate;
    int pktSndPeriod;
    int pktRcvPeriod;
    int pktSndPeriodOld;
    int pktRcvPeriodOld;
    int byteAvailSndBuf;
    int byteAvailRcvBuf;
    int msRoundTripTime;
    int pktFlightSize;
    int msRoundTripTimeOld;
    int pktFlightSizeOld;
} SRT_TRACEBSTATS;

SRT_API int srt_bistats(SRTSOCKET u, SRT_TRACEBSTATS* stats, int cleared);

/* Startup/cleanup */
SRT_API int srt_startup(void);
SRT_API int srt_cleanup(void);

/* Version string */
SRT_API const char* srt_getversion(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_SRTC_H */