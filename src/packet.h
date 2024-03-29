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
** 11 - 15      Reserved
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

#pragma pack(push, 1)
typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint8_t             packetNum[3];               // 0x01 - Packet number (24-bit)

    uint16_t            status;                     // 0x04 - Status bits

    uint16_t            rawWindDir;                 // 0x06 - Raw ADC wind direction

    int16_t             rawTemperature;             // 0x08 - Raw I2C TMP117 value
    uint32_t            rawICPPressure;             // 0x0A - Raw pressure from icp10125
    uint16_t            rawHumidity;                // 0x0E - Raw I2C SHT4x value
    uint8_t             rawALS_UV[6];               // 0x10 - Raw I2C LTR390 ALS & UV value
    uint16_t            rawBatteryVolts;            // 0x16 - Raw I2C value for battery V
    uint16_t            rawBatteryPercentage;       // 0x18 - Raw I2C value for battery %

    uint16_t            rawRainfall;                // 0x1A - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x1C - Raw wind speed
    uint16_t            rawWindGust;                // 0x1E - Raw wind gust speed
}
weather_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint8_t             reserved;                   // 0x01 - Reserved

    uint16_t            status;                     // 0x02 - Status bits

    uint16_t            sleepHours;                 // 0x04 - How many hours is the weather station sleeping for?
    uint16_t            rawBatteryVolts;            // 0x06 - The last raw I2C value for battery V
    uint16_t            rawBatteryPercentage;       // 0x08 - The last raw I2C value for batttery percentage
    uint8_t             rawALS_UV[6];               // 0x0A - The last raw light level

    uint8_t             padding[16];
}
sleep_packet_t;

typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint8_t             reserved;                   // 0x01 - Reserved

    uint16_t            status;                     // 0x02 - Status bits

    uint8_t             padding[28];
}
watchdog_packet_t;
#pragma pack(pop)

weather_packet_t *  getWeatherPacket(void);
sleep_packet_t *    getSleepPacket(void);
watchdog_packet_t * getWatchdogPacket(void);

#endif
