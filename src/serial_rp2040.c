#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "serial_rp2040.h"

#include "gpio_def.h"

void initSerial(uart_inst_t * uart) {
    gpio_set_function(DEBUG_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(DEBUG_PIN_RX, GPIO_FUNC_UART);

	uart_init(uart, 115200);
}

void deinitSerial(uart_inst_t * uart) {
    uart_deinit(uart);

    gpio_set_function(DEBUG_PIN_TX, GPIO_FUNC_SIO);
    gpio_set_function(DEBUG_PIN_RX, GPIO_FUNC_SIO);

    gpio_set_dir(DEBUG_PIN_TX, GPIO_OUT);
    gpio_set_dir(DEBUG_PIN_RX, GPIO_OUT);

    gpio_disable_pulls(DEBUG_PIN_TX);    
    gpio_disable_pulls(DEBUG_PIN_RX);    

    gpio_put(DEBUG_PIN_TX, false);
    gpio_put(DEBUG_PIN_RX, false);
}
