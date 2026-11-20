/*
 * Status LED - WS2812 RGB strip driver (RMT based)
 *
 * Drives a chain of two WS2812s on a single data pin:
 *   LED 0 -> WiFi link state
 *   LED 1 -> SRT connection / streaming state
 *
 * NOTE: set_pixel(g, r, b) - green channel first (WS2812 GRB order)
 *
 * Blink patterns (to keep the existing single-state API):
 *   BOOT         both amber, 200ms blink
 *   WIFI_WAIT    LED0 blue slow blink (500ms), LED1 dark
 *   SRT_WAIT     LED0 green solid, LED1 amber fast blink (500ms)
 *   SRT_ON       both green solid
 *   ERROR        both red, slow double-flash (200ms on / 800ms off)
 *   OFF          both dark
 *
 * Color cheat sheet:
 *   GREEN  = streaming (SRT connected)
 *   BLUE   = waiting for WiFi
 *   AMBER  = booting / connecting to SRT
 *   RED    = error
 *   OFF    = LED disabled / not initialized
 */

#include "status_led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "sdkconfig.h"

#ifndef CONFIG_STATUS_LED_ENABLE
#define CONFIG_STATUS_LED_ENABLE 1
#endif

#ifndef CONFIG_STATUS_LED_GPIO
#define CONFIG_STATUS_LED_GPIO 48
#endif

#ifndef CONFIG_STATUS_LED_COUNT
#define CONFIG_STATUS_LED_COUNT 2
#endif

#ifndef CONFIG_STATUS_LED_SCAN
#define CONFIG_STATUS_LED_SCAN 0
#endif

#ifndef CONFIG_STATUS_LED_BRIGHTNESS
#define CONFIG_STATUS_LED_BRIGHTNESS 128   /* 0-255, 128 = ~50% brightness */
#endif

static const char *TAG = "STATUS_LED";

/* Brightness scale: 0-255, default 128 (~50%). All color values are
   multiplied by this factor / 255 to dim the LEDs. */
#define LED_SCALE(c) (((uint32_t)(c) * CONFIG_STATUS_LED_BRIGHTNESS) / 255)

#if !CONFIG_STATUS_LED_ENABLE

/* Disabled: all calls no-ops */
bool status_led_init(void) { return false; }
void status_led_set(led_state_t state) { (void)state; }
void status_led_update(void) {}
void status_led_pin_scan(void) {}

#else

#define LED_COUNT CONFIG_STATUS_LED_COUNT

/* ------------------------------------------------------------------ */
/* RMT / WS2812 plumbing                                                */
/* ------------------------------------------------------------------ */
static struct {
    bool     initialized;
    rmt_channel_handle_t  chan;
    rmt_encoder_handle_t  encoder;
    rmt_transmit_config_t tx_cfg;
    gpio_num_t            gpio;
    uint8_t               pixels[LED_COUNT][3];   /* G, R, B */
    uint8_t               shadow[LED_COUNT][3];   /* last transmitted */
    bool                  has_shadow;
} s_ws = {
    .tx_cfg = { .loop_count = 0,
                .flags = { .eot_level = 0, .queue_nonblocking = 1 } }
};

/* WS2812 bit encoding: 10 MHz tick (100 ns) -> 0 = 300/900 ns, 1 = 900/300 ns */
#define WS2812_T0H  3
#define WS2812_T0L  9
#define WS2812_T1H  9
#define WS2812_T1L  3

static bool create_channel(gpio_num_t gpio)
{
    rmt_tx_channel_config_t cfg = {
        .gpio_num          = gpio,
        .clk_src           = RMT_CLK_SRC_DEFAULT,
        .resolution_hz     = 10 * 1000 * 1000,   /* 10 MHz, 100 ns per tick */
        .mem_block_symbols = 64,
        .trans_queue_depth = 4,
        .intr_priority     = 0,
    };
    if (rmt_new_tx_channel(&cfg, &s_ws.chan) != ESP_OK) {
        return false;
    }
    if (rmt_enable(s_ws.chan) != ESP_OK) {
        rmt_del_channel(s_ws.chan);
        s_ws.chan = NULL;
        return false;
    }
    return true;
}

static bool create_encoder(void)
{
    rmt_bytes_encoder_config_t enc_cfg = {
        .bit0 = {
            .level0 = 1, .duration0 = WS2812_T0H,
            .level1 = 0, .duration1 = WS2812_T0L,
        },
        .bit1 = {
            .level0 = 1, .duration0 = WS2812_T1H,
            .level1 = 0, .duration1 = WS2812_T1L,
        },
        .flags = { .msb_first = 1 },
    };
    return rmt_new_bytes_encoder(&enc_cfg, &s_ws.encoder) == ESP_OK;
}

bool status_led_init(void)
{
    if (s_ws.initialized) return true;

    s_ws.gpio = (gpio_num_t)CONFIG_STATUS_LED_GPIO;

    if (!create_channel(s_ws.gpio)) {
        ESP_LOGE(TAG, "rmt_new_tx_channel(GPIO%d) failed", s_ws.gpio);
        return false;
    }
    if (!create_encoder()) {
        ESP_LOGE(TAG, "rmt_new_bytes_encoder failed");
        rmt_disable(s_ws.chan);
        rmt_del_channel(s_ws.chan);
        s_ws.chan = NULL;
        return false;
    }

    memset(s_ws.pixels, 0, sizeof(s_ws.pixels));
    s_ws.has_shadow = false;
    s_ws.initialized = true;

    ESP_LOGI(TAG, "WS2812 status strip: %d LED(s) on GPIO%d", LED_COUNT, s_ws.gpio);
    return true;
}

static void refresh(void)
{
    if (!s_ws.initialized) return;

    uint8_t flat[LED_COUNT * 3];
    memcpy(flat, s_ws.pixels, sizeof(s_ws.pixels));

    /* WS2812 wants G,R,B per pixel - our array is already G,R,B */
    (void)rmt_transmit(s_ws.chan, s_ws.encoder, flat, sizeof(flat), &s_ws.tx_cfg);
    memcpy(s_ws.shadow, s_ws.pixels, sizeof(s_ws.shadow));
    s_ws.has_shadow = true;
}

static void set_pixel(int idx, uint8_t g, uint8_t r, uint8_t b)
{
    if (idx < 0 || idx >= LED_COUNT) return;
    s_ws.pixels[idx][0] = g;
    s_ws.pixels[idx][1] = r;
    s_ws.pixels[idx][2] = b;
}

/* ------------------------------------------------------------------ */
/* Pattern engine                                                       */
/* ------------------------------------------------------------------ */
static void pattern_ms(led_state_t st, int *on, int *off)
{
    switch (st) {
    case LED_STATE_OFF:       *on = 0;    *off = 0;    break;
    case LED_STATE_BOOT:     *on = 200;  *off = 200;  break;  /* slower boot blink */
    case LED_STATE_WIFI_WAIT:  *on = 500;  *off = 500;  break;  /* blue pulse = waiting for WiFi */
    case LED_STATE_SRT_WAIT:  *on = 500;  *off = 500;  break;  /* medium amber pulse = SRT connecting */
    case LED_STATE_SRT_ON:
    case LED_STATE_ON:        *on = 1;    *off = 0;    break;  /* solid green = streaming */
    case LED_STATE_ERROR:     *on = 200;  *off = 800;  break;  /* slow red double flash */
    default:                  *on = 0;    *off = 0;    break;
    }
}

static void apply_state(led_state_t state, bool phase_on)
{
    switch (state) {
    case LED_STATE_BOOT:
        /* both amber blink. set_pixel(g, r, b): G=180, R=60, B=0 -> AMBER */
        if (phase_on) {
            set_pixel(0, LED_SCALE(180), LED_SCALE(60), 0);
            set_pixel(1, LED_SCALE(180), LED_SCALE(60), 0);
        } else {
            set_pixel(0, 0, 0, 0);
            set_pixel(1, 0, 0, 0);
        }
        break;

    case LED_STATE_WIFI_WAIT:
        /* LED0 blue slow blink = waiting for WiFi; LED1 dark */
        set_pixel(0, 0, 0, phase_on ? LED_SCALE(255) : 0);   /* G=0, R=0, B=255 -> BLUE */
        set_pixel(1, 0, 0, 0);
        break;

    case LED_STATE_SRT_WAIT:
        /* LED0 green solid = WiFi OK; LED1 amber fast blink = SRT connecting */
        set_pixel(0, LED_SCALE(255), 0, 0);                  /* G=255, R=0, B=0 -> GREEN */
        if (phase_on) { set_pixel(1, LED_SCALE(180), LED_SCALE(60), 0); }   /* G=180, R=60, B=0 -> AMBER */
        else          { set_pixel(1, 0, 0, 0); }
        break;

    case LED_STATE_SRT_ON:
    case LED_STATE_ON:
        /* both green solid = streaming */
        set_pixel(0, LED_SCALE(255), 0, 0);                  /* G=255, R=0, B=0 -> GREEN */
        set_pixel(1, LED_SCALE(255), 0, 0);                  /* G=255, R=0, B=0 -> GREEN */
        break;

    case LED_STATE_ERROR:
        /* both red double flash, handled by phase windows */
        set_pixel(0, 0, phase_on ? LED_SCALE(255) : 0, 0);   /* G=0, R=255/0, B=0 -> RED */
        set_pixel(1, 0, phase_on ? LED_SCALE(255) : 0, 0);   /* G=0, R=255/0, B=0 -> RED */
        break;

    case LED_STATE_OFF:
    default:
        set_pixel(0, 0, 0, 0);
        set_pixel(1, 0, 0, 0);
        break;
    }
}

static struct {
    led_state_t state;
    TickType_t  last_toggle;
} s_led = {
    .state = LED_STATE_OFF,
    .last_toggle = 0
};

void status_led_set(led_state_t state)
{
    if (state >= LED_STATE_MAX) state = LED_STATE_OFF;
    s_led.state = state;
    s_led.last_toggle = 0;   /* force immediate re-render */
}

void status_led_update(void)
{
    if (!s_ws.initialized) return;

    int on_ms, off_ms;
    pattern_ms(s_led.state, &on_ms, &off_ms);

    if (on_ms <= 0 && off_ms <= 0) {          /* OFF */
        memset(s_ws.pixels, 0, sizeof(s_ws.pixels));
        refresh();
        return;
    }
    if (on_ms > 0 && off_ms <= 0) {           /* solid */
        apply_state(s_led.state, true);
        refresh();
        return;
    }

    TickType_t now = xTaskGetTickCount();
    int period = on_ms + off_ms;
    int duty_a = on_ms;
    bool is_error = (s_led.state == LED_STATE_ERROR);

    if (is_error) { period = 1000; duty_a = 100; }

    int elapsed = (int)((now - s_led.last_toggle) * portTICK_PERIOD_MS);
    if (elapsed >= period) {
        s_led.last_toggle = now;
        elapsed = 0;
    }

    bool phase_on;
    if (is_error) {
        /* on at 0..100 and 200..300, off otherwise */
        phase_on = (elapsed < duty_a) || (elapsed >= 200 && elapsed < 300);
    } else {
        phase_on = (elapsed < duty_a);
    }

    apply_state(s_led.state, phase_on);
    refresh();
}

/* ------------------------------------------------------------------ */
/* Diagnostic pin scan                                                  */
/* Walks GPIOs that are safe on this board, filling the strip with     */
/* white so an addressable LED is unmistakable.                        */
/* Skips 26-37 (octal PSRAM), 19-20 (USB D-/D+), 43-44 (console UART). */
/* ------------------------------------------------------------------ */
#if CONFIG_STATUS_LED_SCAN
static const int s_scan_pins[] = {
    45, 46, 47, 48
};
#endif /* CONFIG_STATUS_LED_SCAN */

void status_led_pin_scan(void)
{
#if CONFIG_STATUS_LED_SCAN
    const int npins = (int)(sizeof(s_scan_pins) / sizeof(s_scan_pins[0]));

    ESP_LOGW(TAG, "=== PIN SCAN START: watch the WS2812s and note which GPIO glows white ===");
    ESP_LOGW(TAG, "=== WS2812 PIN SCAN: %d pins (45-48), 1s on + 0.5s pause each ===", npins);
    fflush(stdout);

    for (int i = 0; i < npins; i++) {
        gpio_num_t gpio_num = (gpio_num_t)s_scan_pins[i];

        /* Rebuild the channel on this candidate pin */
        if (s_ws.initialized) {
            rmt_disable(s_ws.chan);
            rmt_del_channel(s_ws.chan);
            s_ws.chan = NULL;
            s_ws.initialized = false;
        }
        if (!create_channel(gpio_num)) {
            continue;
        }
        if (!create_encoder()) {
            rmt_disable(s_ws.chan);
            rmt_del_channel(s_ws.chan);
            s_ws.chan = NULL;
            continue;
        }
        s_ws.gpio = gpio_num;
        s_ws.initialized = true;

        /* Fill both LEDs with a unique color per GPIO so you can tell by color */
        uint8_t g_r = 0, g_g = 0, g_b = 0;
        switch (gpio_num) {
        case 45: g_r = 255; g_g = 0;   g_b = 0;   break; /* RED */
        case 46: g_r = 0;   g_g = 255; g_b = 0;   break; /* GREEN */
        case 47: g_r = 0;   g_g = 0;   g_b = 255; break; /* BLUE */
        case 48: g_r = 255; g_g = 255; g_b = 255; break; /* WHITE */
        default: g_r = 128; g_g = 128; g_b = 128; break;
        }

        /* Flash that color twice, 1s on each */
        for (int b = 0; b < 2; b++) {
            set_pixel(0, g_g, g_r, g_b);
            set_pixel(1, g_g, g_r, g_b);
            refresh();
            vTaskDelay(pdMS_TO_TICKS(1000));  /* 1 second on - long enough to see */
            set_pixel(0, 0, 0, 0);
            set_pixel(1, 0, 0, 0);
            refresh();
            vTaskDelay(pdMS_TO_TICKS(200));   /* 200ms off */
        }

        /* Extra pause between GPIOs so you can tell which color was which */
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_LOGW(TAG, "[scan] GPIO %d done (%s) - did the LED glow %s?",
                 gpio_num,
                 gpio_num == 45 ? "RED" : gpio_num == 46 ? "GREEN" :
                 gpio_num == 47 ? "BLUE" : "WHITE",
                 gpio_num == 45 ? "RED" : gpio_num == 46 ? "GREEN" :
                 gpio_num == 47 ? "BLUE" : "WHITE");
    }

    /* Restore the configured pin */
    if (s_ws.initialized) {
        rmt_disable(s_ws.chan);
        rmt_del_channel(s_ws.chan);
        s_ws.chan = NULL;
        s_ws.initialized = false;
    }
    status_led_init();

    ESP_LOGW(TAG, "=== PIN SCAN COMPLETE ===");
    ESP_LOGW(TAG, "Tell me which GPIO number lit up and I will set STATUS_LED_GPIO to it.");
#endif
}

#endif /* CONFIG_STATUS_LED_ENABLE */