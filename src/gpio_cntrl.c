
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "gpio_def.h"

void deInitGPOIs(void) {
    /*
    ** Claim GPIOs as outputs and drive them all low...
    */
    uint pinMask = 
            (1 << I2C0_SDA_ALT_PIN) | 
            (1 << I2C0_SLK_ALT_PIN) | 
            (1 << NRF24L01_SPI_PIN_CE) | 
            (1 << NRF24L01_SPI_PIN_CSN) | 
            (1 << NRF24L01_SPI_PIN_MISO) | 
            (1 << NRF24L01_SPI_PIN_MOSI) | 
            (1 << NRF24L01_SPI_PIN_SCK) |
            (1 << I2C0_POWER_PIN_0);

    gpio_init_mask(pinMask);
    gpio_set_dir_out_masked(pinMask);

    gpio_disable_pulls(I2C0_SDA_ALT_PIN);    
    gpio_disable_pulls(I2C0_SLK_ALT_PIN);    
    gpio_disable_pulls(NRF24L01_SPI_PIN_CE);    
    gpio_disable_pulls(NRF24L01_SPI_PIN_CSN);    
    gpio_disable_pulls(NRF24L01_SPI_PIN_MISO);    
    gpio_disable_pulls(NRF24L01_SPI_PIN_MOSI);    
    gpio_disable_pulls(NRF24L01_SPI_PIN_SCK);    
    gpio_disable_pulls(I2C0_POWER_PIN_0);    

    gpio_clr_mask(pinMask);
}

void deInitGPOIsAndDebugPins(void) {
    deInitGPOIs();

    /*
    ** Claim GPIOs as outputs and drive them all low...
    */
    uint pinMask = 
            (1 << SCOPE_DEBUG_PIN_0) | 
            (1 << SCOPE_DEBUG_PIN_1) | 
            (1 << SCOPE_DEBUG_PIN_2) |
            (1 << ONBAORD_LED_PIN);

    gpio_init_mask(pinMask);
    gpio_set_dir_out_masked(pinMask);

    gpio_disable_pulls(SCOPE_DEBUG_PIN_0);    
    gpio_disable_pulls(SCOPE_DEBUG_PIN_1);    
    gpio_disable_pulls(SCOPE_DEBUG_PIN_2);    
    gpio_disable_pulls(ONBAORD_LED_PIN);    

    gpio_clr_mask(pinMask);
}
