#include "pico/stdlib.h"

#include "led_utils.h"

const uint LED_PIN = PICO_DEFAULT_LED_PIN;

void setupLEDPin(void)
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

void turnOn(int LED_ID)
{
    gpio_put(LED_PIN, 1);
}

void turnOff(int LED_ID)
{
    gpio_put(LED_PIN, 0);
}

void toggleLED(int LED_ID)
{
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
