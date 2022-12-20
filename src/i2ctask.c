#include <stdlib.h>
#include <stdio.h>

#include "scheduler.h"
#include "i2c_rp2040.h"

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
    //i2cWrite();
}