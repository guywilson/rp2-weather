#include "pico/stdlib.h"

#ifndef __INCL_GPIO_DEF
#define __INCL_GPIO_DEF

#define I2C_SDA_ALT_PIN				16
#define I2C_SLK_ALT_PIN				17

#define PWM_RAIN_GAUGE_PIN          14
#define PWM_ANEMOMETER_PIN          15

#define PIO_PIN_ANEMOMETER          18
#define PIO_PIN_RAIN_GAUGE          12

#define NRF24L01_SPI_PIN_CE          6
#define NRF24L01_SPI_PIN_CSN         5
#define NRF24L01_SPI_PIN_MOSI        3
#define NRF24L01_SPI_PIN_MISO        4
#define NRF24L01_SPI_PIN_SCK         2

#define I2C0_POWER_PIN              22

#define ONBAORD_LED_PIN             PICO_DEFAULT_LED_PIN

#endif
