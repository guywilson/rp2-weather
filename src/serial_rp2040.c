#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "serial_rp2040.h"

#include "gpio_def.h"

void irqTx(void) {
    //
}

void irqRx(void) {

}

void setupSerial(void)
{
	uart_init(uart0, 115200);

    gpio_set_function(DEBUG_PIN_TX, GPIO_FUNC_UART);
    gpio_set_function(DEBUG_PIN_RX, GPIO_FUNC_UART);
}
