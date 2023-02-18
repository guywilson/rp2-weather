#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "scheduler.h"
#include "taskdef.h"
#include "spi_rp2040.h"
#include "rtc_rp2040.h"
#include "nRF24L01.h"

static char         szTemp[32];

void _powerUp(spi_inst_t * spi) {
    uint8_t             statusReg;
    uint8_t             configReg;

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiReadByte(spi, NRF24L01_SPI_PIN_CSN, &configReg, false);

    configReg |= 0x02;

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configReg, false);
}

void _powerDown(spi_inst_t * spi) {
    uint8_t             statusReg;
    uint8_t             configReg;

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiReadByte(spi, NRF24L01_SPI_PIN_CSN, &configReg, false);

    configReg &= 0xFD;

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configReg, false);
}

int _transmit(
            spi_inst_t * spi, 
            uint8_t * buf, 
            bool requestACK)
{
    uint8_t             command;
    uint8_t             statusReg;

    if (requestACK) {
        command = NRF24L01_CMD_W_TX_PAYLOAD;
    }
    else {
        command = NRF24L01_CMD_W_TX_PAYLOAD_NOACK;
    }

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_FLUSH_TX, &statusReg, false);
    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, command, &statusReg, true);
    spiWriteData(spi, NRF24L01_SPI_PIN_CSN, buf, 32, false);

    // sprintf(szTemp, "St: 0x%02X\n", statusReg);
    // uart_puts(uart0, szTemp);

    /*
    ** Pulse the CE line for > 10us to enable 
    ** the device in TX mode to send the data
    */
    gpio_put(NRF24L01_SPI_PIN_CE, true);

    rtcDelay(15U);

    gpio_put(NRF24L01_SPI_PIN_CE, false);

    /*
    ** Clear the TX_DS bit...
    */
    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_STATUS, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, (statusReg & 0xDF), false);

    return 0;
}

int nRF24L01_setup(spi_inst_t * spi) {
    int             error = 0;
    uint8_t         statusReg = 0;
    uint8_t         configReg = 0;
    uint8_t         configData[8];
    uint8_t         txAddr[5]; // {0xE0, 0xE0, 0xF1, 0xF1, 0xE0};

	/*
	** SPI CSn
	*/
	gpio_init(NRF24L01_SPI_PIN_CSN);
	gpio_set_dir(NRF24L01_SPI_PIN_CSN, true);
	gpio_put(NRF24L01_SPI_PIN_CSN, true);

	/*
	** SPI CE
	*/
	gpio_init(NRF24L01_SPI_PIN_CE);
	gpio_set_dir(NRF24L01_SPI_PIN_CE, true);
	gpio_put(NRF24L01_SPI_PIN_CE, false);

	gpio_init(NRF24L01_SPI_PIN_IRQ);
	gpio_set_dir(NRF24L01_SPI_PIN_IRQ, false);				// SPI IRQ

	gpio_set_function(NRF24L01_SPI_PIN_MOSI, GPIO_FUNC_SPI);	// SPI TX
	gpio_set_function(NRF24L01_SPI_PIN_MISO, GPIO_FUNC_SPI);	// SPI RX
	gpio_set_function(NRF24L01_SPI_PIN_SCK, GPIO_FUNC_SPI);	// SPI SCK

    sleep_ms(100);

    configData[0] = 0x7E;       // CONFIG register
    configData[1] = 0x00;       // EN_AA register
    configData[2] = 0x01;       // EN_RXADDR register
    configData[3] = 0x03;       // SETUP_AW register
    configData[4] = 0x00;       // SETUP_RETR register
    configData[5] = 40;         // RF_CH register
    configData[6] = 0x07;       // RF_SETUP register
    configData[7] = 0x70;       // STATUS register

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configData[0], false);

    sleep_ms(2);

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_R_REGISTER | NRF24L01_REG_CONFIG, &statusReg, true);
    spiReadByte(spi, NRF24L01_SPI_PIN_CSN, &configReg, false);

    sprintf(szTemp, "Cfg: 0x%02X\n", configReg);
    uart_puts(uart0, szTemp);

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_EN_AA, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configData[1], false);

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_EN_RXADDR, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configData[2], false);

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_SETUP_AW, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configData[3], false);

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_SETUP_RETR, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configData[4], false);

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RF_CH, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configData[5], false);

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RF_SETUP, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configData[6], false);

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_STATUS, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, configData[7], false);

    sprintf(szTemp, "St: 0x%02X\n", statusReg);
    uart_puts(uart0, szTemp);

    txAddr[0] = 'A';
    txAddr[1] = 'Z';
    txAddr[2] = '4';
    txAddr[3] = '3';
    txAddr[4] = '8';

    spiWriteReadByte(
                spi, 
                NRF24L01_SPI_PIN_CSN, 
                NRF24L01_CMD_W_REGISTER | NRF24L01_REG_RX_ADDR_PO, 
                &statusReg, 
                true);
    spiWriteData(spi, NRF24L01_SPI_PIN_CSN, txAddr, 5, false);

    spiWriteReadByte(
                spi, 
                NRF24L01_SPI_PIN_CSN, 
                NRF24L01_CMD_W_REGISTER | NRF24L01_REG_TX_ADDR, 
                &statusReg, 
                true);
    spiWriteData(spi, NRF24L01_SPI_PIN_CSN, txAddr, 5, false);

    sprintf(szTemp, "St: 0x%02X\n", statusReg);
    uart_puts(uart0, szTemp);

    /*
    ** Activate additional features...
    */
    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_ACTIVATE, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, 0x73, false);

    /*
    ** Enable NOACK transmit...
    */
    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_W_REGISTER | NRF24L01_REG_FEATURE, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, 0x01, false);

//    _powerUp(spi);

    return error;
}

int nRF24L01_transmit_buffer(
            spi_inst_t * spi, 
            uint8_t * buf, 
            int length, 
            bool requestACK)
{
    int             error = 0;
    uint8_t         buffer[32];

    if (length > 32) {
        return PICO_ERROR_GENERIC;
    }

    memset(buffer, 0, 32);
    memcpy(buffer, buf, length);

    _transmit(spi, buffer, requestACK);

    return error;
}

int nRF24L01_transmit_string(
            spi_inst_t * spi, 
            char * pszText, 
            bool requestACK)
{
    int             error = 0;
    uint8_t         buffer[32];
    int             strLength;

    strLength = strlen(pszText);

    if (strLength > 32) {
        strLength = 32;
    }

    memset(buffer, 0, 32);
    memcpy(buffer, pszText, strLength);

    _transmit(spi, buffer, requestACK);

    return error;
}

int nRF24L01_receive(spi_inst_t * spi, uint8_t * buffer, int length) {
    int         error = 0;

    return error;
}
