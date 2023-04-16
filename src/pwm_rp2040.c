#include <stdint.h>
#include <pico/stdlib.h>

#include "hardware/pwm.h"

#define PWM_ANEMOMETER_CLOCK_PIN              20
#define PWM_RAIN_GAUGE_CLOCK_PIN              21

void pwmInit() {
    uint            slice;

    gpio_set_function(PWM_ANEMOMETER_CLOCK_PIN, GPIO_FUNC_PWM);
    gpio_set_function(PWM_RAIN_GAUGE_CLOCK_PIN, GPIO_FUNC_PWM);

    slice = pwm_gpio_to_slice_num(PWM_ANEMOMETER_CLOCK_PIN);

    /*
    ** Create a PWM clock with a 50% duty cycle @ 400Hz... 
    */
    pwm_set_clkdiv(slice, 256.0);
    pwm_set_wrap(slice, 1220);
    pwm_set_chan_level(slice, PWM_CHAN_A, 610);
    pwm_set_chan_level(slice, PWM_CHAN_B, 610);
    
    pwm_set_enabled(slice, true);
}
