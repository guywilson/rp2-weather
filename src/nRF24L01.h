#ifndef __INCL_NRF24L01
#define __INCL_NRF24L01

/*
** PIN definitions
*/
#define NRF24L01_SPI_PIN_CE                         26
#define NRF24L01_SPI_PIN_CSN                         5
#define NRF24L01_SPI_PIN_IRQ                        28
#define NRF24L01_SPI_PIN_MOSI                        3
#define NRF24L01_SPI_PIN_MISO                        4
#define NRF24L01_SPI_PIN_SCK                         2

/*
** nRF24L01 commands
*/
#define NRF24L01_CMD_R_REGISTER                     0x00
#define NRF24L01_CMD_W_REGISTER                     0x20
#define NRF24L01_CMD_R_RX_PAYLOAD                   0x61
#define NRF24L01_CMD_W_TX_PAYLOAD                   0xA0
#define NRF24L01_CMD_FLUSH_TX                       0xE1
#define NRF24L01_CMD_FLUSH_RX                       0xE2
#define NRF24L01_CMD_REUSE_TX_PL                    0xE3
#define NRF24L01_CMD_ACTIVATE                       0x50
#define NRF24L01_CMD_R_RX_PL_WID                    0x60
#define NRF24L01_CMD_W_ACK_PAYLOAD                  0xA8
#define NRF24L01_CMD_W_TX_PAYLOAD_NOACK             0xB0
#define NRF24L01_CMD_NOP                            0xFF

/*
** nRF24L01 register map
*/
#define NRF24L01_REG_CONFIG                         0x00
#define NRF24L01_REG_EN_AA                          0x01
#define NRF24L01_REG_EN_RXADDR                      0x02
#define NRF24L01_REG_SETUP_AW                       0x03
#define NRF24L01_REG_SETUP_RETR                     0x04
#define NRF24L01_REG_RF_CH                          0x05
#define NRF24L01_REG_RF_SETUP                       0x06
#define NRF24L01_REG_STATUS                         0x07
#define NRF24L01_REG_OBSERVE_TX                     0x08
#define NRF24L01_REG_CD                             0x09
#define NRF24L01_REG_RX_ADDR_PO                     0x0A
#define NRF24L01_REG_RX_ADDR_P1                     0x0B
#define NRF24L01_REG_RX_ADDR_P2                     0x0C
#define NRF24L01_REG_RX_ADDR_P3                     0x0D
#define NRF24L01_REG_RX_ADDR_P4                     0x0E
#define NRF24L01_REG_RX_ADDR_P5                     0x0F
#define NRF24L01_REG_TX_ADDR                        0x10
#define NRF24L01_REG_RX_PW_P0                       0x11
#define NRF24L01_REG_RX_PW_P1                       0x12
#define NRF24L01_REG_RX_PW_P2                       0x13
#define NRF24L01_REG_RX_PW_P3                       0x14
#define NRF24L01_REG_RX_PW_P4                       0x15
#define NRF24L01_REG_RX_PW_P5                       0x16
#define NRF24L01_REG_DYNPD                          0x1C
#define NRF24L01_REG_FEATURE                        0x1D

int nRF24L01_setup(spi_inst_t * spi); 
int nRF24L01_transmit_buffer(
            spi_inst_t * spi, 
            uint8_t * buf, 
            int length, 
            bool requestACK);
int nRF24L01_transmit_string(
            spi_inst_t * spi, 
            char * pszText, 
            bool requestACK);
int nRF24L01_receive(
            spi_inst_t * spi, 
            uint8_t * buffer, 
            int length);

#endif
