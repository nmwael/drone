/*
 * Status LED - onboard LED status indication
 *
 * Simple blink-code generator (polled from the main loop, no task needed):
 *   fast blink  -> booting / starting up
 *   slow blink  -> waiting for WiFi
 *   med blink   -> WiFi OK, waiting for SRT
 *   solid on    -> SRT streaming
 *   double flash -> error
 */

#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_BOOT,        /* fast blink */
    LED_STATE_WIFI_WAIT,   /* slow blink */
    LED_STATE_SRT_WAIT,    /* medium blink */
    LED_STATE_SRT_ON,      /* solid on */
    LED_STATE_ERROR,       /* double-flash repeat */
    LED_STATE_ON,           /* solid on (raw) */
    LED_STATE_MAX
} led_state_t;

/* Configure the GPIO. Safe to call twice. */
bool status_led_init(void);

/* Set the current status; takes effect on the next status_led_update(). */
void status_led_set(led_state_t state);

/* Drive the LED; call once per main-loop iteration. */
void status_led_update(void);

/* Diagnostic: walk every safe GPIO, blinking each one so the user can
   identify which pin the onboard LED is on. Runs ~15s, then returns. */
void status_led_pin_scan(void);

#endif /* STATUS_LED_H */