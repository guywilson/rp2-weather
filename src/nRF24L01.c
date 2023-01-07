#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/irq.h"
#include "scheduler.h"
#include "taskdef.h"
#include "spi_rp2040.h"
#include "nRF24L01.h"

static char         szTemp[32];

int nRF24L01_setup(spi_inst_t * spi, uint csPin) {
    int             error = 0;
    uint8_t         statusReg = 0;
    uint8_t         configData[8];
    uint8_t         txAddr[5]; // {0xE0, 0xE0, 0xF1, 0xF1, 0xE0};

    configData[0] = NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG;
    configData[1] = 0x7A;       // CONFIG register
    configData[2] = 0x00;       // EN_AA register
    configData[3] = 0x01;       // EN_RXADDR register
    configData[4] = 0x03;       // SETUP_AW register
    configData[5] = 0x00;       // SETUP_SETR register
    configData[6] = 0x02;       // RF_CH register
    configData[7] = 0x03;       // RF_SETUP register

    spiWriteReadByte(spi, 5, configData[0], &statusReg, true);
    spiWriteData(spi, 5, &configData[1], 7, false);

    sprintf(szTemp, "St: 0x%02X\n", statusReg);
    uart_puts(uart0, szTemp);

    txAddr[0] = 0x0E;
    txAddr[1] = 0x0E;
    txAddr[2] = 0xF1;
    txAddr[3] = 0xF1;
    txAddr[4] = 0x0E;

    spiWriteReadByte(
                spi, 
                5, 
                NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RX_ADDR_PO, 
                &statusReg, 
                true);
    spiWriteData(spi, 5, txAddr, 5, false);

    sprintf(szTemp, "St: 0x%02X\n", statusReg);
    uart_puts(uart0, szTemp);

    return error;
}

int nRF24L01_transmit(
            spi_inst_t * spi, 
            uint csPin, 
            uint8_t * buffer, 
            int length, 
            bool requestACK)
{
    int         error = 0;
    uint8_t     command;

    if (requestACK) {
        command = 0xA0;
    }
    else {

    }

    return error;
}

int nRF24L01_receive(spi_inst_t * spi, uint csPin, uint8_t * buffer, int length) {
    int         error = 0;

    return error;
}
