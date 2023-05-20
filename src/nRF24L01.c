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
#include "gpio_def.h"

#define NRF24L01_REMOTE_ADDRESS         "AZ438"
#define NRF24L01_LOCAL_ADDRESS          "AZ437"

#define NRF24L01_RF_CHANNEL             76


typedef struct {
    uint8_t         CONFIG;
    uint8_t         EN_AA;
    uint8_t         EN_RXADDR;
    uint8_t         SETUP_AW;
    uint8_t         SETUP_RETR;
    uint8_t         RF_CH;
    uint8_t         RF_SETUP;
    uint8_t         STATUS;
    uint8_t         RXADDR_P[6][5];
    uint8_t         TXADDR[5];
    uint8_t         RX_PW_P[6];
    uint8_t         FIFO_STATUS;
    uint8_t         DYNPD;
    uint8_t         FEATURE;
}
nRF24_reg_map_t;

static nRF24_reg_map_t  _registerMap;

size_t _getAddressWidth() {
    return (size_t)(_registerMap.SETUP_AW + 2);
}

void _updateRegMap(uint8_t reg, uint8_t value) {
    switch (reg) {
        case NRF24L01_REG_CONFIG:
            _registerMap.CONFIG = value;
            break;

        case NRF24L01_REG_EN_AA:
            _registerMap.EN_AA = value;
            break;

        case NRF24L01_REG_EN_RXADDR:
            _registerMap.EN_RXADDR = value;
            break;

        case NRF24L01_REG_SETUP_AW:
            _registerMap.SETUP_AW = value;
            break;

        case NRF24L01_REG_SETUP_RETR:
            _registerMap.SETUP_RETR = value;
            break;

        case NRF24L01_REG_RF_CH:
            _registerMap.RF_CH = value;
            break;

        case NRF24L01_REG_RF_SETUP:
            _registerMap.RF_SETUP = value;
            break;

        case NRF24L01_REG_STATUS:
            _registerMap.STATUS = value;
            break;

        case NRF24L01_REG_RX_ADDR_PO:
        case NRF24L01_REG_RX_ADDR_P1:
        case NRF24L01_REG_RX_ADDR_P2:
        case NRF24L01_REG_RX_ADDR_P3:
        case NRF24L01_REG_RX_ADDR_P4:
        case NRF24L01_REG_RX_ADDR_P5:
            break;

        case NRF24L01_REG_RX_PW_P0:
        case NRF24L01_REG_RX_PW_P1:
        case NRF24L01_REG_RX_PW_P2:
        case NRF24L01_REG_RX_PW_P3:
        case NRF24L01_REG_RX_PW_P4:
        case NRF24L01_REG_RX_PW_P5:
            _registerMap.RX_PW_P[reg - NRF24L01_REG_RX_PW_P0] = value;
            break;

        case NRF24L01_REG_FIFO_STATUS:
            _registerMap.FIFO_STATUS = value;
            break;

        case NRF24L01_REG_DYNPD:
            _registerMap.DYNPD = value;
            break;

        case NRF24L01_REG_FEATURE:
            _registerMap.FEATURE = value;
            break;
    }
}

void nRF24L01_writeRegister(spi_inst_t * spi, uint8_t reg, uint8_t value, uint8_t * pStatusReg) {
    int         bytesTransferred;

    bytesTransferred = spiWriteReadByte(
                            spi, 
                            NRF24L01_SPI_PIN_CSN, 
                            NRF24L01_CMD_W_REGISTER | reg, 
                            pStatusReg, 
                            true);

    if (!bytesTransferred) {
        return;
    }

    bytesTransferred = spiWriteByte(
                            spi, 
                            NRF24L01_SPI_PIN_CSN, 
                            value, 
                            false);

    if (bytesTransferred) {
        _updateRegMap(reg, value);
    }
}

uint8_t nRF24L01_readRegister(spi_inst_t * spi, uint8_t reg, uint8_t * pStatusReg) {
    uint8_t             value;

    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_R_REGISTER | reg, pStatusReg, true);
    spiReadByte(spi, NRF24L01_SPI_PIN_CSN, &value, false);

    return value;
}

void _setRxAddress(spi_inst_t * spi, int pipe, const char * pszAddress) {
    uint8_t         statusReg;

    memcpy(&_registerMap.RXADDR_P[pipe][0], pszAddress, _getAddressWidth());

    spiWriteReadByte(
                spi, 
                NRF24L01_SPI_PIN_CSN, 
                NRF24L01_CMD_W_REGISTER | (NRF24L01_REG_RX_ADDR_PO + pipe), 
                &statusReg, 
                true);
    spiWriteData(spi, NRF24L01_SPI_PIN_CSN, &_registerMap.RXADDR_P[pipe][0], _getAddressWidth(), false);
}

void _setTxAddress(spi_inst_t * spi, const char * pszAddress) {
    uint8_t         statusReg;

    memcpy(&_registerMap.TXADDR[0], pszAddress, _getAddressWidth());

    spiWriteReadByte(
                spi, 
                NRF24L01_SPI_PIN_CSN, 
                NRF24L01_CMD_W_REGISTER | NRF24L01_REG_TX_ADDR, 
                &statusReg, 
                true);
    spiWriteData(spi, NRF24L01_SPI_PIN_CSN, &_registerMap.TXADDR[0], _getAddressWidth(), false);
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

	gpio_set_function(NRF24L01_SPI_PIN_MOSI, GPIO_FUNC_SPI);	// SPI TX
	gpio_set_function(NRF24L01_SPI_PIN_MISO, GPIO_FUNC_SPI);	// SPI RX
	gpio_set_function(NRF24L01_SPI_PIN_SCK, GPIO_FUNC_SPI);	    // SPI SCK

    sleep_ms(100);

    nRF24L01_writeRegister(
                    spi, 
                    NRF24L01_REG_CONFIG, 
                    NRF24L01_CFG_MASK_MAX_RT |
                    NRF24L01_CFG_MASK_RX_DR |
                    NRF24L01_CFG_MASK_TX_DS |
                    NRF24L01_CFG_CRC_2_BYTE, 
                    &statusReg);

    sleep_ms(2);

    nRF24L01_writeRegister(spi, NRF24L01_REG_EN_AA, 0x00, &statusReg);
    nRF24L01_writeRegister(spi, NRF24L01_REG_EN_RXADDR, 0x01, &statusReg);
    nRF24L01_writeRegister(spi, NRF24L01_REG_SETUP_AW, 0x03, &statusReg);
    nRF24L01_writeRegister(spi, NRF24L01_REG_SETUP_RETR, 0x00, &statusReg);

    /*
    ** Set the RF channel to use...
    */
    nRF24L01_writeRegister(
                    spi, 
                    NRF24L01_REG_RF_CH, 
                    NRF24L01_RF_CHANNEL, 
                    &statusReg);

    nRF24L01_writeRegister(
                    spi, 
                    NRF24L01_REG_RF_SETUP, 
                    NRF24L01_RF_SETUP_RF_POWER_HIGH | 
                    NRF24L01_RF_SETUP_RF_LNA_GAIN_OFF | 
                    NRF24L01_RF_SETUP_DATA_RATE_1MBPS, 
                    &statusReg);

    nRF24L01_writeRegister(
                    spi, 
                    NRF24L01_REG_STATUS, 
                    NRF24L01_STATUS_CLEAR_MAX_RT |
                    NRF24L01_STATUS_CLEAR_RX_DR |
                    NRF24L01_STATUS_CLEAR_TX_DS, 
                    &statusReg);

    _setRxAddress(spi, 0, NRF24L01_LOCAL_ADDRESS);
    _setTxAddress(spi, NRF24L01_REMOTE_ADDRESS);

    /*
    ** Activate additional features...
    */
    spiWriteReadByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_CMD_ACTIVATE, &statusReg, true);
    spiWriteByte(spi, NRF24L01_SPI_PIN_CSN, NRF24L01_ACTIVATE_SPECIAL_BYTE, false);

    /*
    ** Enable NOACK transmit & dynamic payload length...
    */
    nRF24L01_writeRegister(
                spi, 
                NRF24L01_REG_FEATURE, 
                NRF24L01_FEATURE_EN_PAYLOAD_WITH_ACK | 
                NRF24L01_FEATURE_EN_TX_NO_ACK |
                NRF24L01_FEATURE_EN_DYN_PAYLOAD_LEN, 
                &statusReg);

    // nRF24L01_writeRegister(
    //             spi, 
    //             NRF24L01_REG_DYNPD, 
    //             0x00, 
    //             &statusReg);

    return error;
}

void nRF24L01_powerUpTx(spi_inst_t * spi) {
    uint8_t         statusReg;

    _registerMap.CONFIG &= ~NRF24L01_CFG_MODE_RX;
    _registerMap.CONFIG |= (NRF24L01_CFG_MODE_TX | NRF24L01_CFG_POWER_UP);
    nRF24L01_writeRegister(spi, NRF24L01_REG_CONFIG, _registerMap.CONFIG, &statusReg);
}

void nRF24L01_powerUpRx(spi_inst_t * spi) {
    uint8_t         statusReg;

    _registerMap.CONFIG |= (NRF24L01_CFG_MODE_RX | NRF24L01_CFG_POWER_UP);
    nRF24L01_writeRegister(spi, NRF24L01_REG_CONFIG, _registerMap.CONFIG, &statusReg);
}

void nRF24L01_powerDown(spi_inst_t * spi) {
    uint8_t         statusReg;

    _registerMap.CONFIG &= ~NRF24L01_CFG_POWER_UP;
    nRF24L01_writeRegister(spi, NRF24L01_REG_CONFIG, _registerMap.CONFIG, &statusReg);
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
