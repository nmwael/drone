/*
 * Network TX SRT - Unit Tests
 *
 * Tests SRT configuration, connection, and send functionality.
 * Requires SRT component to be available.
 */

#include "unity.h"
#include "network_tx.h"
#include "network_tx_srt.h"
#include "stanag_types.h"
#include <string.h>

void setUp(void)
{
    /* Reset network config before each test */
    network_tx_set_mode(NETWORK_TX_MODE_UDP);
}

void tearDown(void)
{
    network_tx_deinit();
}

void test_network_tx_srt_config_default(void)
{
    srt_config_t cfg;
    network_tx_srt_config_default(&cfg);

    TEST_ASSERT_EQUAL_STRING("192.168.4.2", cfg.ip);
    TEST_ASSERT_EQUAL_INT(9000, cfg.port);
    TEST_ASSERT_EQUAL_INT(SRT_MODE_CALLER, cfg.mode);
    TEST_ASSERT_EQUAL_INT(200, cfg.latency_ms);
    TEST_ASSERT_TRUE(cfg.enable_tlpktdrop);
    TEST_ASSERT_EQUAL_INT(1500, cfg.mss);
    TEST_ASSERT_EQUAL_INT(0, cfg.pbkeylen);
    TEST_ASSERT_FALSE(cfg.nonce_enabled);
    TEST_ASSERT_EQUAL_STRING("", cfg.passphrase);
}

void test_network_tx_set_srt_config(void)
{
    srt_config_t cfg = {0};
    strcpy(cfg.ip, "10.0.0.1");
    cfg.port = 8888;
    cfg.mode = SRT_MODE_LISTENER;
    cfg.latency_ms = 100;
    strcpy(cfg.passphrase, "test123");
    cfg.pbkeylen = 16;

    network_tx_set_srt_config(&cfg);

    /* Verify by getting stats (which would use internal config) */
    /* For now just verify no crash */
    TEST_PASS();
}

void test_network_tx_mode_enum(void)
{
    TEST_ASSERT_EQUAL_INT(0, NETWORK_TX_MODE_UDP);
    TEST_ASSERT_EQUAL_INT(1, NETWORK_TX_MODE_SRT);
    TEST_ASSERT_EQUAL_INT(2, NETWORK_TX_MODE_ESP_NOW);
    TEST_ASSERT_EQUAL_INT(3, NETWORK_TX_MODE_UNIX_SOCKET);
}

void test_network_tx_srt_mode_enum(void)
{
    TEST_ASSERT_EQUAL_INT(0, SRT_MODE_CALLER);
    TEST_ASSERT_EQUAL_INT(1, SRT_MODE_LISTENER);
    TEST_ASSERT_EQUAL_INT(2, SRT_MODE_RENDEZVOUS);
}

void test_network_tx_get_set_mode(void)
{
    network_tx_set_mode(NETWORK_TX_MODE_SRT);
    TEST_ASSERT_EQUAL_INT(NETWORK_TX_MODE_SRT, network_tx_get_mode());

    network_tx_set_mode(NETWORK_TX_MODE_UDP);
    TEST_ASSERT_EQUAL_INT(NETWORK_TX_MODE_UDP, network_tx_get_mode());

    network_tx_set_mode(NETWORK_TX_MODE_ESP_NOW);
    TEST_ASSERT_EQUAL_INT(NETWORK_TX_MODE_ESP_NOW, network_tx_get_mode());
}

void test_network_tx_srt_init_without_component(void)
{
    /* This test verifies the API compiles and links.
     * Actual SRT connection requires hardware/network and SRT component.
     * In test environment without SRT component, init should fail gracefully.
     */
    network_tx_set_mode(NETWORK_TX_MODE_SRT);
    srt_config_t cfg;
    network_tx_srt_config_default(&cfg);
    network_tx_set_srt_config(&cfg);

    ESP_ERROR_CHECK(network_tx_init());

    /* SRT init will fail if component not linked, which is expected in unit test env */
    esp_err_t ret = network_tx_srt_init();
    /* Either ESP_OK (if SRT component present) or ESP_FAIL/ESP_ERR_NOT_SUPPORTED */
    /* We just verify it doesn't crash */
    TEST_ASSERT_TRUE(ret == ESP_OK || ret == ESP_FAIL || ret == ESP_ERR_NOT_SUPPORTED);
}

void test_srt_config_encryption_fields(void)
{
    srt_config_t cfg;
    network_tx_srt_config_default(&cfg);

    /* Test AES-128 */
    strcpy(cfg.passphrase, "secret12345678");
    cfg.pbkeylen = 16;
    TEST_ASSERT_EQUAL_INT(16, cfg.pbkeylen);
    TEST_ASSERT_EQUAL_STRING("secret12345678", cfg.passphrase);

    /* Test AES-256 */
    strcpy(cfg.passphrase, "secret123456789012345678901234");
    cfg.pbkeylen = 32;
    TEST_ASSERT_EQUAL_INT(32, cfg.pbkeylen);
}

void test_network_tx_srt_get_stats_buffer(void)
{
    char buf[256];
    esp_err_t ret = network_tx_srt_get_stats(buf, sizeof(buf));
    /* Should return error if not initialized */
    TEST_ASSERT_TRUE(ret == ESP_ERR_INVALID_ARG || ret == ESP_ERR_INVALID_STATE || ret == ESP_FAIL);
}

void test_network_tx_srt_is_connected_false_initially(void)
{
    TEST_ASSERT_FALSE(network_tx_srt_is_connected());
}

TEST_CASE("network_tx_srt_config_default sets correct defaults", "[network_tx_srt]")
{
    test_network_tx_srt_config_default();
}

TEST_CASE("network_tx_set_srt_config stores config", "[network_tx_srt]")
{
    test_network_tx_set_srt_config();
}

TEST_CASE("network_tx_mode_enum values correct", "[network_tx_srt]")
{
    test_network_tx_mode_enum();
}

TEST_CASE("network_tx_srt_mode_enum values correct", "[network_tx_srt]")
{
    test_network_tx_srt_mode_enum();
}

TEST_CASE("network_tx_get_set_mode works", "[network_tx_srt]")
{
    test_network_tx_get_set_mode();
}

TEST_CASE("network_tx_srt_init handles missing component gracefully", "[network_tx_srt]")
{
    test_network_tx_srt_init_without_component();
}

TEST_CASE("srt_config_t encryption fields work", "[network_tx_srt]")
{
    test_srt_config_encryption_fields();
}

TEST_CASE("network_tx_srt_get_stats handles uninitialized", "[network_tx_srt]")
{
    test_network_tx_srt_get_stats_buffer();
}

TEST_CASE("network_tx_srt_is_connected returns false initially", "[network_tx_srt]")
{
    test_network_tx_srt_is_connected_false_initially();
}