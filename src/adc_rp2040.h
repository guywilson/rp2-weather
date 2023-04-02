#include <stdint.h>
#include <stdbool.h>

#include "scheduler.h"

#ifndef __INCL_ADC_RP2040
#define __INCL_ADC_RP2040

#define ADC_CHANNEL_NOT_CONNECTED           0
#define ADC_CHANNEL_WIND_DIR                1
#define ADC_CHANNEL_BATTERY_TEMPERATURE     2
#define ADC_CHANNEL_BATTERY_VOLTAGE         3

#define ADC_CHANNEL_INTERNAL_TEMPERATURE    4

typedef struct {
    uint16_t            adcWindDir;
    uint16_t            adcBatteryVoltage;
    uint16_t            adcBatteryTemperature;
    uint16_t            adcRP2040Temperature;
}
adc_samples_t;

void    adcInit();
void    taskADC(PTASKPARM p);
#endif
