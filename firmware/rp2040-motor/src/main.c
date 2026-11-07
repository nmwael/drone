/*
 * RP2040 Motor Control Main Entry
 * PID Velocity Control + Encoder Feedback + RC PWM Input + UART Command
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "hardware/dma.h"

#include "motor_pid.h"
#include "encoder.h"
#include "uart_cmd.h"
#include "rc_pwm_input.h"

#define UART_ID uart0
#define UART_BAUD 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

static const char *TAG = "MOTOR";

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);  // Wait for USB CDC

    printf("\n=== RP2040 Motor Control Starting ===\n");
    printf("Free heap: %d bytes\n", (int)get_free_heap());

    // Initialize UART for commands from C3
    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);
    irq_set_enabled(UART0_IRQ, true);

    // Initialize motor PWM via PIO
    motor_pid_init();
    encoder_init();
    rc_pwm_input_init();

    // Start command processing task on core 1
    multicore_launch_core1(uart_cmd_task);

    printf("Motor control running!\n");

    // Main loop on core 0 - motor PID update
    while (true) {
        motor_pid_update();      // Run PID at 1kHz
        encoder_update();        // Read encoders
        rc_pwm_input_update();   // Read RC PWM inputs
        sleep_ms(1);             // ~1kHz loop
    }

    return 0;
}