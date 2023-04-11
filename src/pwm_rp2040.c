#include <stdint.h>
#include <pico/stdlib.h>

#include "hardware/pwm.h"

#define PWM_ANEMOMETER_PIN              20

void pwmInit() {
    uint            slice;

    gpio_set_function(PWM_ANEMOMETER_PIN, GPIO_FUNC_PWM);

    slice = pwm_gpio_to_slice_num(PWM_ANEMOMETER_PIN);

    pwm_set_clkdiv(slice, 256.0);
    pwm_set_wrap(slice, 10849);
    pwm_set_gpio_level(PWM_ANEMOMETER_PIN, 128);
    
    pwm_set_enabled(slice, true);
}
