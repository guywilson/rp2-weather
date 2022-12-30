#include <stdint.h>

#ifndef __INCL_I2C_RP2040
#define __INCL_I2C_RP2040

#define I2C_SDA_HOLD                38

#define TMP117_ADDRESS                  0x48

#define TMP117_REG_CONFIG               0x01
#define TMP117_REG_TEMP                 0x00
#define TMP117_REG_TEMP_OFFSET          0x07
#define TMP117_REG_DEVICE_ID            0x0F


typedef enum {
    I2C_BAUD_1KHZ   = 0xC35024F8,
    I2C_BAUD_2KHZ   = 0x61A8927C,
    I2C_BAUD_4KHZ   = 0x30D4493E,
    I2C_BAUD_8KHZ   = 0x186A249F,
    I2C_BAUD_16KHZ  = 0x0C36124F,
    I2C_BAUD_32KHZ  = 0x061B0927,
    I2C_BAUD_64KHZ  = 0x030E0493,
    I2C_BAUD_100KHZ = 0x01F402EE,
    I2C_BAUD_200KHZ = 0x00FA0177,
    I2C_BAUD_400KHZ = 0x007E00BB
}
i2c_baud;

typedef struct {
    volatile uint8_t    txBuffer[32];
    volatile int        txIndex;
    int                 txBufferLen;

    volatile uint8_t    rxBuffer[32];
    volatile int        rxIndex;
}
I2C_device;

typedef struct {
    volatile uint8_t *  buffer;
    volatile int        bufferLen;
}
I2C_RX;

void setupTMP117();
void setupI2C(i2c_baud baud);
void i2cWrite(uint8_t addr, uint8_t * data, int length);
void i2cWriteByte(uint8_t addr, uint8_t data);
void i2cRead(uint8_t addr);

#endif
