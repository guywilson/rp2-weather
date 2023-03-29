#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_ADC_RP2040
#define __INCL_ADC_RP2040

#define ADC_INPUT_WIND_DIR              0
#define ADC_INPUT_BATTERY_VOLTAGE       1
#define ADC_INPUT_BATTERY_TEMPERATURE   2
#define ADC_INPUT_NOT_CONNECTED         3

#define ADC_INPUT_INTERNAL_TEMPERATURE  4

void    adcInit();

#endif
