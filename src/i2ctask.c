#include <stdlib.h>
#include <stdio.h>

#include "scheduler.h"
#include "i2c_rp2040.h"

#define TMP177_ADDRESS                  0x48

#define TMP117_REG_CONFIG               0x01
#define TMP117_REG_TEMP                 0x00
#define TMP117_REG_TEMP_OFFSET          0x07
#define TMP117_REG_DEVICE_ID            0x0F

void TempResultTask(PTASKPARM p) {
    int16_t         tempRegister;
    double          temp;
    I2C_RX *        pRxStruct;

    pRxStruct = (I2C_RX *)p;

    tempRegister = (int16_t)((int16_t)(pRxStruct->buffer[1] << 8) + (int16_t)pRxStruct->buffer[0]);
    temp = (double)tempRegister * 0.0078125;

    printf("Got temp: %.2f", temp);
}

void TempTriggerTask(PTASKPARM p) {
    uint8_t             configData[3];

    /*
    ** One-shot conversion, 8 sample average...
    */
    configData[0] = TMP117_REG_CONFIG;
    configData[1] = 0x0C;
    configData[2] = 0x20;

    i2cWrite(TMP177_ADDRESS, &configData, 3);

    i2cWriteByte(TMP177_ADDRESS, TMP117_REG_CONFIG);

    i2cRead(TMP177_ADDRESS);
}