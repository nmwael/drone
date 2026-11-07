/*
 * RP2040 #2 ToF Main Entry
 * 4x VL53L5CX ToF Array (Front-Left, Left, Rear-Left, Aux) - 4MB Flash
 * I2C via TCA9548A mux, UART streaming to ESP32-C3
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/uart.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"

#include "vl53l5cx_4x.h"
#include "uart_cmd.h"
#include "i2c_mux.h"

#define UART_ID uart0
#define UART_BAUD 1000000
#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define I2C_PORT i2c0
#define I2C_SDA 4
#define I2C_SCL 5
#define I2C_BAUD 1000000

#define MUX_ADDR 0x70

static const char *TAG = "TOF2";

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);

    printf("\n=== RP2040 #2 ToF Array (4MB Flash) Starting ===\n");
    printf("4x VL53L5CX: Front-Left, Left, Rear-Left, Aux\n");

    uart_init(UART_ID, UART_BAUD);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_fifo_enabled(UART_ID, true);
    irq_set_enabled(UART0_IRQ, true);

    i2c_init(I2C_PORT, I2C_BAUD);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    i2c_mux_init(I2C_PORT, MUX_ADDR);
    vl53l5cx_4x_init();

    multicore_launch_core1(uart_cmd_task);

    printf("ToF Array #2 running! Streaming point clouds @ 30 Hz\n");

    while (true) {
        vl53l5cx_4x_read_all();
        vl53l5cx_4x_stream_uart();
        sleep_ms(33);
    }

    return 0;
}