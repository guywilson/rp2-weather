#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

#include "gpio_def.h"
#include "utils.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

void setupLEDPin(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, false);
}

void setupDebugPin(void) {
    gpio_init(DEBUG_ENABLE_PIN);
    gpio_set_function(DEBUG_ENABLE_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(DEBUG_ENABLE_PIN, false);
    gpio_pull_down(DEBUG_ENABLE_PIN);
}

void turnOn(int LED_ID) {
    gpio_put(LED_PIN, 1);
}

void turnOff(int LED_ID) {
    gpio_put(LED_PIN, 0);
}

void toggleLED(int LED_ID) {
	static unsigned char state = 0;
	
	if (!state) {
		turnOn(LED_PIN);
		state = 1;
	}
	else {
		turnOff(LED_PIN);
		state = 0;
	}
}

inline bool isDebugActive(void) {
    return (gpio_get(DEBUG_ENABLE_PIN) != 0 ? true : false);
}

inline int16_t copyI2CReg_int16(uint8_t * reg) {
    return ((int16_t)((((int16_t)reg[0]) << 8) | (int16_t)reg[1]));
}

inline uint16_t copyI2CReg_uint16(uint8_t * reg) {
    return ((uint16_t)((((uint16_t)reg[0]) << 8) | (uint16_t)reg[1]));
}
