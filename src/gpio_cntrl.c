
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "gpio_def.h"

void initGPIOs(void) {
    /*
    ** I2C bus pins...
    */
    gpio_set_function(I2C0_SDA_ALT_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SLK_ALT_PIN, GPIO_FUNC_I2C);

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

    gpio_init(I2C0_POWER_PIN_0);
    gpio_set_dir(I2C0_POWER_PIN_0, GPIO_OUT);

    gpio_set_drive_strength(I2C0_POWER_PIN_0, GPIO_DRIVE_STRENGTH_4MA);
}

void initDebugPins(void) {
    gpio_init(SCOPE_DEBUG_PIN_0);
    gpio_init(SCOPE_DEBUG_PIN_1);
    gpio_init(SCOPE_DEBUG_PIN_2);

    gpio_set_dir(SCOPE_DEBUG_PIN_0, true);
    gpio_set_dir(SCOPE_DEBUG_PIN_1, true);
    gpio_set_dir(SCOPE_DEBUG_PIN_2, true);
}

void deInitGPIOs(void) {
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

void deInitGPIOsAndDebugPins(void) {
    deInitGPIOs();

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
