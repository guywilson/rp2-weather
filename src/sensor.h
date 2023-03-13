#include <stdint.h>

#include "scheduler.h"
#include "hardware/i2c.h"

#ifndef __INCL_I2C_SENSOR
#define __INCL_I2C_SENSOR

typedef struct {
    uint32_t            chipID;                     // 0x00 - ID of the RP2040
    uint16_t            messageID;                  // 0x04 - Incrementing message num

    uint16_t            rawBatteryVolts;            // 0x06 - Raw ADC value for battery V
    uint16_t            rawSolarVolts;              // 0x08 - Raw ADC value for solar V
    int16_t             rawChipTemperature;         // 0x0A - Raw ADC value of RP2040 temp
    int16_t             rawBatteryTemperature;      // 0x0C - Raw ADC value for battery temp

    int16_t             rawTemperature;             // 0x0E - Raw I2C TMP117 value
    uint16_t            rawPressure;                // 0x10 - Raw I2C pressure value
    uint16_t            rawHumidity;                // 0x12 - Raw I2C SHT4x value
    uint16_t            rawLux;                     // 0x14 - Raw I2C lux value
    uint16_t            rawRainfall;                // 0x16 - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x18 - Raw anemometer count
    uint16_t            rawWindDir;                 // 0x1A - Raw wind direction
}
weather_packet_t;

int     initSensors(i2c_inst_t * i2c);
void    taskReadTemp(PTASKPARM p);
void    taskReadHumidity_step1(PTASKPARM p);
void    taskReadHumidity_step2(PTASKPARM p);
void    taskReadPressure(PTASKPARM p);

#endif
