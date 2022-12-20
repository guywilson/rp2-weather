#include <stdint.h>

#ifndef __INCL_I2C_RP2040
#define __INCL_I2C_RP2040

#define I2C_SDA_HOLD                38

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
    uint8_t *       buffer;
    int             bufferLen;
}
I2C_RX;

void setupI2C(i2c_baud baud);
void i2cWrite(uint8_t addr, uint8_t * data, int length);
void i2cWriteByte(uint8_t addr, uint8_t data);
void i2cRead(uint8_t addr);

#endif
