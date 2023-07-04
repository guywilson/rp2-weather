#include <stdint.h>
#include <stdbool.h>

#ifndef __INCL_PACKET
#define __INCL_PACKET

#define PACKET_ID_WEATHER                           0x55
#define PACKET_ID_SLEEP                             0xAA
#define PACKET_ID_WATCHDOG                          0x96

/*
** Status bits:
** 
** E = I2C Not acknowledged OR timeout error
** G = LC709203 I2C Generic error on last read
** T = LC709203 I2C Timeout error on last read
** C = LC7092023 CRC error
** 0 = No error, all OK
**
** 0            TMP117      E
** 1            SHT4X       E
** 2            ICP10125    E
** 3            LTR390 ALS  E
** 4            LTR390 UVI  E
** 5 - 7        LC709203 BV T|G|C
** 8 - 10       LC709203 BP T|G|C
** 11 - 13      LC709203 BT T|G|C
** 14 - 15      Reserved
*/
#define STATUS_BITS_TMP117_I2C_ERROR                0x0001
#define STATUS_BITS_SHT4X_I2C_ERROR                 0x0002
#define STATUS_BITS_ICP10125_I2C_ERROR              0x0004
#define STATUS_BITS_LTR390_ALS_I2C_ERROR            0x0008
#define STATUS_BITS_LTR390_UVI_I2C_ERROR            0x0010
#define STATUS_BITS_LC709203_BV_I2C_CRC_ERROR       0x0020
#define STATUS_BITS_LC709203_BV_I2C_GEN_ERROR       0x0040
#define STATUS_BITS_LC709203_BV_I2C_TIMEOUT_ERROR   0x0080
#define STATUS_BITS_LC709203_BP_I2C_CRC_ERROR       0x0100
#define STATUS_BITS_LC709203_BP_I2C_GEN_ERROR       0x0200
#define STATUS_BITS_LC709203_BP_I2C_TIMEOUT_ERROR   0x0400
#define STATUS_BITS_LC709203_BT_I2C_CRC_ERROR       0x0800
#define STATUS_BITS_LC709203_BT_I2C_GEN_ERROR       0x1000
#define STATUS_BITS_LC709203_BT_I2C_TIMEOUT_ERROR   0x2000

#pragma pack(push, 1)
typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint16_t            status;                     // 0x01 - Status bits

    uint16_t            rawWindDir;                 // 0x03 - Raw ADC wind direction

    int16_t             rawTemperature;             // 0x05 - Raw I2C TMP117 value
    uint32_t            rawICPPressure;             // 0x07 - Raw pressure from icp10125
    uint16_t            rawICPTemperature;          // 0x0B - Raw temperature from icp10125
    uint16_t            rawHumidity;                // 0x0D - Raw I2C SHT4x value
    uint8_t             rawALS_UV[5];               // 0x0F - Raw I2C LTR390 ALS & UV value
    uint16_t            rawBatteryVolts;            // 0x14 - Raw I2C value for battery V
    uint16_t            rawBatteryPercentage;       // 0x16 - Raw I2C value for battery %
    uint16_t            rawBatteryTemperature;      // 0x18 - Raw I2C value for battery temp

    uint16_t            rawRainfall;                // 0x1A - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x1C - Raw wind speed
    uint16_t            rawWindGust;                // 0x1E - Raw wind gust speed
}
weather_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint16_t            status;                     // 0x01 - Status bits

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

    uint16_t            status;                     // 0x01 - Status bits

    uint8_t             padding[29];
}
watchdog_packet_t;
#pragma pack(pop)

weather_packet_t *  getWeatherPacket(void);
sleep_packet_t *    getSleepPacket(void);
watchdog_packet_t * getWatchdogPacket(void);

#endif
