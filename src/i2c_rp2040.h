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

int i2cWriteRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length);
int i2cReadRegister(
            i2c_inst_t *i2c, 
            const uint addr, 
            const uint8_t reg, 
            uint8_t * data, 
            const uint8_t length);

void setupI2C(i2c_inst_t * i2c, i2c_baud baud);
void i2cWrite(i2c_inst_t * i2c, uint8_t addr, uint8_t * data, int length);
void i2cWriteByte(i2c_inst_t * i2c, uint8_t addr, uint8_t data);
void i2cRead(i2c_inst_t * i2c, uint8_t addr);

#endif
