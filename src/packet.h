#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_PACKET
#define __INCL_PACKET

#pragma pack(push, 1)
typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    char                packetID[2];                // 0x00 - Identify this as a weather packet

    uint32_t            chipID;                     // 0x02 - ID of the RP2040

    uint16_t            rawWindDir;                 // 0x06 - Raw ADC wind direction
    uint16_t            rawBatteryVolts;            // 0x08 - Raw ADC value for battery V
    uint16_t            rawBatteryTemperature;      // 0x0A - Raw ADC value for battery temp

    int16_t             rawTemperature;             // 0x0C - Raw I2C TMP117 value
    uint32_t            rawICPPressure;             // 0x0E - Raw pressure from icp10125
    uint16_t            rawICPTemperature;          // 0x12 - Raw temperature from icp10125
    uint16_t            rawHumidity;                // 0x14 - Raw I2C SHT4x value
    uint16_t            rawLux;                     // 0x16 - Raw I2C veml7700 lux value

    uint16_t            rawRainfall;                // 0x18 - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x1A - Raw anemometer count
}
weather_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    char                packetID[2];                // 0x00 - Identify this as a sleep packet

    uint32_t            chipID;                     // 0x02 - ID of the RP2040

    uint16_t            sleepHours;                 // 0x06 - How many hours is the weather station sleeping for?
    uint16_t            rawBatteryVolts;            // 0x08 - The last raw ADC value for battery V
    uint16_t            rawBatteryTemperature;      // 0x0A - The last raw ADC value for battery temp
    uint16_t            rawLux;                     // 0x0C - The last raw light level
}
sleep_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    char                packetID[2];                // 0x00 - Identify this as a sleep packet

    uint32_t            chipID;                     // 0x02 - ID of the RP2040
}
watchdog_packet_t;
#pragma pack(pop)

#endif
