#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_PACKET
#define __INCL_PACKET

#define PACKET_ID_WEATHER                           0x55
#define PACKET_ID_SLEEP                             0xAA
#define PACKET_ID_WATCHDOG                          0x96

#pragma pack(push, 1)
typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint16_t            chipID;                     // 0x01 - ID of the RP2040

    uint16_t            rawWindDir;                 // 0x02 - Raw ADC wind direction
    uint16_t            rawVSYSVoltage;             // 0x04 - Raw ADC VSYS input voltage

    int16_t             rawTemperature;             // 0x06 - Raw I2C TMP117 value
    uint32_t            rawICPPressure;             // 0x08 - Raw pressure from icp10125
    uint16_t            rawICPTemperature;          // 0x0C - Raw temperature from icp10125
    uint16_t            rawHumidity;                // 0x0E - Raw I2C SHT4x value
    uint8_t             rawALS_UV[5];               // 0x10 - Raw I2C LTR390 ALS & UV value
    uint16_t            rawBatteryVolts;            // 0x15 - Raw I2C value for battery V
    uint16_t            rawBatteryPercentage;       // 0x17 - Raw I2C value for battery %
    uint16_t            rawBatteryTemperature;      // 0x19 - Raw I2C value for battery temp

    uint16_t            rawRainfall;                // 0x1B - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x1D - Raw anemometer count
}
weather_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint16_t            chipID;                     // 0x01 - ID of the RP2040

    uint16_t            sleepHours;                 // 0x03 - How many hours is the weather station sleeping for?
    uint16_t            rawBatteryVolts;            // 0x05 - The last raw I2C value for battery V
    uint16_t            rawBatteryPercentage;       // 0x07 - The last raw I2C value for batttery percentage
    uint16_t            rawBatteryTemperature;      // 0x09 - The last raw I2C value for battery temp
    uint8_t             rawALS_UV[5];               // 0x0B - The last raw light level

    uint8_t             padding[16];
}
sleep_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint16_t            chipID;                     // 0x01 - ID of the RP2040

    uint8_t             padding[29];
}
watchdog_packet_t;
#pragma pack(pop)

weather_packet_t *  getWeatherPacket(void);
sleep_packet_t *    getSleepPacket(void);
watchdog_packet_t * getWatchdogPacket(void);

#endif
