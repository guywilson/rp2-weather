#ifndef __INCL_SERIAL_RP2040
#define __INCL_SERIAL_RP2040

#define UART_TX_PIN 0
#define UART_RX_PIN 1

void initSerial(uart_inst_t * uart);
void deinitSerial(uart_inst_t * uart);

#endif
