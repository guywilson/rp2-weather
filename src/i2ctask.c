#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "math.h"

#include "hardware/i2c.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2ctask.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"

void TempResultTask(PTASKPARM p) {
    int16_t         tempRegister;
    double          temp;
    I2C_RX *        pRxStruct;
    char            szTemp[32];

    pRxStruct = (I2C_RX *)p;

    tempRegister = (int16_t)((int16_t)(pRxStruct->buffer[1] << 8) + (int16_t)pRxStruct->buffer[0]);
    temp = (double)tempRegister * 0.0078125;

    sprintf(szTemp, "Got temp: %.2f\n", temp);

    uart_puts(uart0, szTemp);

    scheduleTaskOnce(TASK_TEMP_TRIGGER, rtc_val_sec(5), NULL);
}

void TaskTriggerTemp(PTASKPARM p) {
    uint8_t             configData[3];

    uart_puts(uart0, "Triggering temperature read...\n");

    /*
    ** One-shot conversion, 8 sample average...
    */
    configData[0] = TMP117_REG_CONFIG;
    configData[1] = 0x0C;
    configData[2] = 0x20;
    i2c_write_timeout_us(i2c0, TMP117_ADDRESS, configData, 3, true, 100);

    scheduleTaskOnce(TASK_TEMP_READ, rtc_val_ms(150), NULL);
}

void TaskReadTemp(PTASKPARM p) {
    uint8_t             tempData[1];
    uint8_t             tempRegister[2];
    int16_t             tempInt;
    double              temp;
    char                szTemp[32];

    tempData[0] = TMP117_REG_TEMP;
    
    i2c_write_timeout_us(i2c0, TMP117_ADDRESS, tempData, 1, true, 100);
    i2c_read_timeout_us(i2c0, TMP117_ADDRESS, tempRegister, 2, true, 100);

    tempInt = (int16_t)((int16_t)(tempRegister[0] << 8) + (int16_t)tempRegister[1]);
    temp = (double)tempInt * 0.0078125;

    sprintf(szTemp, "Got temp: %.2f\n", temp);
    uart_puts(uart0, szTemp);
}
