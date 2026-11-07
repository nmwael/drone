/*
 * RP2040 Rear ToF + Aux Main Entry
 * VL53L5CX x2 (Rear, Right) + UART Command
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"

#include "vl53l5cx.h"
#include "uart_cmd.h"

#define UART_ID uart0
#define UART_BAUD 115200
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define I2C_BAUD 1000000

#define I2C_PORT_1 i2c1
#define I2C_SDA_1 2
#define I2C_SCL_1 3

static const char *TAG = "REAR_TOF";

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);

    printf("\n=== RP2040 Rear ToF Starting ===\n");

    // Initialize UART for commands from C3
    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);
    irq_set_enabled(UART0_IRQ, true);

    // Initialize I2C for ToF sensors
    i2c_init(I2C_PORT, I2C_BAUD);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    i2c_init(I2C_PORT_1, I2C_BAUD);
    gpio_set_function(I2C_SDA_1, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_1, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_1);
    gpio_pull_up(I2C_SCL_1);

    // Initialize VL53L5CX sensors (rear + right)
    vl53l5cx_init_rear();
    vl53l5cx_init_right();

    // Start command processing on core 1
    multicore_launch_core1(uart_cmd_task);

    printf("Rear ToF running!\n");

    // Main loop on core 0 - ToF scanning at 30Hz
    while (true) {
        vl53l5cx_read_rear();
        vl53l5cx_read_right();
        sleep_ms(33);  // ~30Hz
    }

    return 0;
}