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
#define STATUS_BITS_TMP117_I2C_ERROR                0x01
#define STATUS_BITS_SHT4X_I2C_ERROR                 0x02
#define STATUS_BITS_ICP10125_I2C_ERROR              0x04
#define STATUS_BITS_LTR390_ALS_I2C_ERROR            0x08
#define STATUS_BITS_LTR390_UVI_I2C_ERROR            0x10
#define STATUS_BITS_MAX17048_BV_I2C_ERROR           0x20
#define STATUS_BITS_MAX17048_BP_I2C_ERROR           0x40
#define STATUS_BITS_MAX17048_BCR_I2C_ERROR          0x80

#pragma pack(push, 1)
typedef struct {                                    // O/S  - Description
                                                    // ----   ---------------------------------
    uint8_t             packetID;                   // 0x00 - Identify this as a weather packet

    uint8_t             packetNum[3];               // 0x01 - Packet number (24-bit)

    uint8_t             status;                     // 0x04 - Status bits

    uint8_t             rawBatteryPercentage;       // 0x05 - Raw I2C value for battery %
    int16_t             rawBatteryChargeRate;       // 0x06 - Raw I2C battery charge rate
    uint16_t            rawBatteryVolts;            // 0x08 - Raw I2C value for battery V

    int16_t             rawTemperature;             // 0x0A - Raw I2C TMP117 value
    uint32_t            rawICPPressure;             // 0x0C - Raw pressure from icp10125
    uint16_t            rawHumidity;                // 0x10 - Raw I2C SHT4x value

    uint16_t            rawRainfall;                // 0x12 - Raw rain sensor count
    uint16_t            rawWindspeed;               // 0x14 - Raw wind speed
    uint16_t            rawWindGust;                // 0x16 - Raw wind gust speed

    uint8_t             padding[8];                 // 0x18
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

    uint8_t             padding[22];
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
