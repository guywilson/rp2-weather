#include <stdint.h>

#include "scheduler.h"
#include "hardware/i2c.h"

#ifndef __INCL_I2C_SENSOR
#define __INCL_I2C_SENSOR

#pragma pack(push, 1)
typedef struct {
    uint32_t            chipID;                     // 0x00 - ID of the RP2040

    uint16_t            rawBatteryVolts;            // 0x04 - Raw ADC value for battery V
    uint16_t            rawBatteryTemperature;      // 0x06 - Raw ADC value for battery temp
    uint16_t            rawChipTemperature;         // 0x08 - Raw ADC value of RP2040 temp

    int16_t             rawTemperature;             // 0x0A - Raw I2C TMP117 value
    uint32_t            rawICPPressure;             // 0x0C - Raw pressure from icp10125
    uint16_t            rawICPTemperature;          // 0x10 - Raw temperature from icp10125
    uint16_t            rawHumidity;                // 0x12 - Raw I2C SHT4x value
    uint16_t            rawLux;                     // 0x14 - Raw I2C veml7700 lux value
    uint16_t            rawRainfall;                // 0x16 - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x18 - Raw anemometer count
    uint16_t            rawWindDir;                 // 0x1A - Raw ADC wind direction
}
weather_packet_t;
#pragma pack(pop)

weather_packet_t *  getWeatherPacket();
int                 initSensors(i2c_inst_t * i2c);
void                taskI2CSensor(PTASKPARM p);

#endif
