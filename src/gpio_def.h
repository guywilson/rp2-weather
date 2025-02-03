/******************************************************************************
**                                                                           **
**                        RASPBERRY PI PICO PINOUT                           **
**                                                                           **
*******************************************************************************
**                                  _____                                    **
**             GPIO  0 (01) -|^^^^^|^^^^^|^^^^^|- (40) VBUS                  **
**             GPIO  1 (02) -|     | USB |     |- (39) VSYS                  **
**             GND     (03) -|     ^^^^^^^     |- (38) GND                   **
**             GPIO  2 (04) -|                 |- (37) 3V3 EN                **
**             GPIO  3 (05) -|                 |- (36) 3V3 (OUT)             **
**             GPIO  4 (06) -|                 |- (35) ADC VREF              **
**             GPIO  5 (07) -|                 |- (34) GPIO 28               **
**             GND     (08) -|                 |- (33) GND                   **
**             GPIO  6 (09) -|                 |- (32) GPIO 27               **
**             GPIO  7 (10) -|    |^^^^^^^|    |- (31) GPIO 26               **
**             GPIO  8 (11) -|    |   RP  |    |- (30) RUN                   **
**             GPIO  9 (12) -|    |  2040 |    |- (29) GPIO 22               **
**             GND     (13) -|    ^^^^^^^^^    |- (28) GND                   **
**             GPIO 10 (14) -|                 |- (27) GPIO 21               **
**             GPIO 11 (15) -|                 |- (26) GPIO 20               **
**             GPIO 12 (16) -|                 |- (25) GPIO 19               **
**             GPIO 13 (17) -|                 |- (24) GPIO 18               **
**             GND     (18) -|                 |- (23) GND                   **
**             GPIO 14 (19) -|                 |- (22) GPIO 17               **
**             GPIO 15 (20) -|_________________|- (21) GPIO 16               **
**                                                                           **
******************************************************************************/

#include "pico/stdlib.h"

#ifndef __INCL_GPIO_DEF
#define __INCL_GPIO_DEF

#define I2C0_SDA_ALT_PIN			16
#define I2C0_SLK_ALT_PIN			17

#define PIO_PIN_ANEMOMETER          11
#define PIO_PIN_RAIN_GAUGE          13

#define NRF24L01_SPI_PIN_CE         10
#define NRF24L01_SPI_PIN_CSN         9
#define NRF24L01_SPI_PIN_MOSI        7
#define NRF24L01_SPI_PIN_MISO        8
#define NRF24L01_SPI_PIN_SCK         6

#define DEBUG_PIN_TX                UART_TX_PIN         // GPIO 0
#define DEBUG_PIN_RX                UART_RX_PIN         // GPIO 1

#define I2C0_POWER_PIN_0            18

#define DEBUG_ENABLE_PIN            12

#define ONBAORD_LED_PIN             PICO_DEFAULT_LED_PIN

#endif
