/*
 * SRT Stub Unit Tests
 *
 * Tests for the minimal SRT implementation (srt_stub.c).
 * Uses Unity test framework (included with ESP-IDF).
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "unity.h"
#include "srt.h"

/* Test setup/teardown */
void setUp(void)
{
    srt_startup();
}

void tearDown(void)
{
    srt_cleanup();
}

/* Test: srt_getversion returns version string */
void test_srt_getversion_returns_string(void)
{
    const char *ver = srt_getversion();
    TEST_ASSERT_NOT_NULL(ver);
    TEST_ASSERT_GREATER_THAN(0, strlen(ver));
    printf("SRT Version: %s\n", ver);
}

/* Test: srt_create_socket returns valid socket */
void test_srt_create_socket_returns_valid(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    TEST_ASSERT_GREATER_THAN(0, sock);
    srt_close(sock);
}

/* Test: srt_create_socket multiple sockets */
void test_srt_create_socket_multiple(void)
{
    SRTSOCKET sock1 = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock1);
    
    SRTSOCKET sock2 = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock2);
    TEST_ASSERT_NOT_EQUAL(sock1, sock2);
    
    srt_close(sock1);
    srt_close(sock2);
}

/* Test: srt_bind works */
void test_srt_bind_works(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int ret = srt_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    srt_close(sock);
}

/* Test: srt_listen works */
void test_srt_listen_works(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9001);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    srt_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    
    int ret = srt_listen(sock, 1);
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    srt_close(sock);
}

/* Test: srt_setsockopt for stream ID */
void test_srt_setsockopt_stream_id(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    const char *stream_id = "publish:live/test_stream:user:pass";
    int ret = srt_setsockopt(sock, 0, SRTO_STREAMID, stream_id, strlen(stream_id));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    /* Verify we can get it back */
    char buf[256];
    int optlen = sizeof(buf);
    ret = srt_getsockopt(sock, 0, SRTO_STREAMID, buf, &optlen);
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    TEST_ASSERT_EQUAL_STRING(stream_id, buf);
    
    srt_close(sock);
}

/* Test: srt_setsockopt for latency */
void test_srt_setsockopt_latency(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    int latency = 200;
    int ret = srt_setsockopt(sock, 0, SRTO_LATENCY, &latency, sizeof(latency));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    int get_latency = 0;
    int optlen = sizeof(get_latency);
    ret = srt_getsockopt(sock, 0, SRTO_LATENCY, &get_latency, &optlen);
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    TEST_ASSERT_EQUAL(latency, get_latency);
    
    srt_close(sock);
}

/* Test: srt_setsockopt for MSS */
void test_srt_setsockopt_mss(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    int mss = 1400;
    int ret = srt_setsockopt(sock, 0, SRTO_MSS, &mss, sizeof(mss));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    int get_mss = 0;
    int optlen = sizeof(get_mss);
    ret = srt_getsockopt(sock, 0, SRTO_MSS, &get_mss, &optlen);
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    TEST_ASSERT_EQUAL(mss, get_mss);
    
    srt_close(sock);
}

/* Test: srt_setsockopt for passphrase */
void test_srt_setsockopt_passphrase(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    const char *passphrase = "secret123";
    int ret = srt_setsockopt(sock, 0, SRTO_PASSPHRASE, passphrase, strlen(passphrase));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    int pbkeylen = 16;
    ret = srt_setsockopt(sock, 0, SRTO_PBKEYLEN, &pbkeylen, sizeof(pbkeylen));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    srt_close(sock);
}

/* Test: srt_sendmsg2 on unconnected socket returns error */
void test_srt_sendmsg2_unconnected(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    const char *data = "test";
    int ret = srt_sendmsg2(sock, data, 4, NULL);
    TEST_ASSERT_EQUAL(SRT_ERROR, ret);
    
    srt_close(sock);
}

/* Test: srt_close works */
void test_srt_close_works(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    int ret = srt_close(sock);
    TEST_ASSERT_EQUAL(SRT_OK, ret);
}

/* Test: srt_error_handling */
void test_srt_error_handling(void)
{
    srt_clearlasterror();
    TEST_ASSERT_EQUAL(SRT_OK, srt_getlasterror());
    
    const char *err_str = srt_getlasterror_str();
    TEST_ASSERT_NOT_NULL(err_str);
}

/* Test: srt_bistats returns valid structure */
void test_srt_bistats_works(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    SRT_TRACEBSTATS stats;
    int ret = srt_bistats(sock, &stats, 0);
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    /* Initial stats should be zero */
    TEST_ASSERT_EQUAL(0, stats.pktSentTotal);
    TEST_ASSERT_EQUAL(0, stats.pktRecvTotal);
    TEST_ASSERT_EQUAL(0, stats.byteSentTotal);
    TEST_ASSERT_EQUAL(0, stats.byteRecvTotal);
    
    srt_close(sock);
}

/* Test: SRT constants are defined */
void test_srt_constants_defined(void)
{
    TEST_ASSERT_EQUAL(1, SRT_VERSION_MAJOR);
    TEST_ASSERT_EQUAL(5, SRT_VERSION_MINOR);
    TEST_ASSERT_EQUAL(7, SRT_VERSION_PATCH);
    
    TEST_ASSERT_EQUAL(0x0000, SRT_CMD_HSREQ);
    TEST_ASSERT_EQUAL(0x0001, SRT_CMD_HSRSP);
    
    TEST_ASSERT_EQUAL(0, SRTO_MODE_CALLER);
    TEST_ASSERT_EQUAL(1, SRTO_MODE_LISTENER);
    TEST_ASSERT_EQUAL(2, SRTO_MODE_RENDEZVOUS);
    
    TEST_ASSERT_EQUAL(0, SRT_OK);
    TEST_ASSERT_EQUAL(-1, SRT_ERROR);
    TEST_ASSERT_EQUAL(-1, SRT_INVALID_SOCK);
}

/* Test: srt_startup and cleanup */
void test_srt_startup_cleanup(void)
{
    int ret = srt_startup();
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    ret = srt_cleanup();
    TEST_ASSERT_EQUAL(SRT_OK, ret);
}

/* Test: srt_setsockopt TLPKTDROP */
void test_srt_setsockopt_tlpktdrop(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    int enable = 1;
    int ret = srt_setsockopt(sock, 0, SRTO_TLPKTDROP, &enable, sizeof(enable));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    enable = 0;
    ret = srt_setsockopt(sock, 0, SRTO_TLPKTDROP, &enable, sizeof(enable));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    srt_close(sock);
}

/* Test: srt_setsockopt enforced encryption */
void test_srt_setsockopt_enforced_encryption(void)
{
    SRTSOCKET sock = srt_create_socket();
    TEST_ASSERT_NOT_EQUAL(SRT_INVALID_SOCK, sock);
    
    int enable = 1;
    int ret = srt_setsockopt(sock, 0, SRTO_ENFORCEDENCRYPTION, &enable, sizeof(enable));
    TEST_ASSERT_EQUAL(SRT_OK, ret);
    
    srt_close(sock);
}

/* Test: SRT_VERSION_VALUE macro */
void test_srt_version_value(void)
{
    uint32_t expected = (SRT_VERSION_MAJOR << 16) | (SRT_VERSION_MINOR << 8) | SRT_VERSION_PATCH;
    TEST_ASSERT_EQUAL(expected, SRT_VERSION_VALUE);
}

/* Main test runner */
#ifndef UNITY_TEST_RUNNER
void app_main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_srt_getversion_returns_string);
    RUN_TEST(test_srt_create_socket_returns_valid);
    RUN_TEST(test_srt_create_socket_multiple);
    RUN_TEST(test_srt_bind_works);
    RUN_TEST(test_srt_listen_works);
    RUN_TEST(test_srt_setsockopt_stream_id);
    RUN_TEST(test_srt_setsockopt_latency);
    RUN_TEST(test_srt_setsockopt_mss);
    RUN_TEST(test_srt_setsockopt_passphrase);
    RUN_TEST(test_srt_sendmsg2_unconnected);
    RUN_TEST(test_srt_close_works);
    RUN_TEST(test_srt_error_handling);
    RUN_TEST(test_srt_bistats_works);
    RUN_TEST(test_srt_constants_defined);
    RUN_TEST(test_srt_startup_cleanup);
    RUN_TEST(test_srt_setsockopt_tlpktdrop);
    RUN_TEST(test_srt_setsockopt_enforced_encryption);
    RUN_TEST(test_srt_version_value);
    
    UNITY_END();
}
#endif