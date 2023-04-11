#include <stdint.h>
#include <pico/stdlib.h>

#include "hardware/pwm.h"

#define PWM_ANEMOMETER_PIN              20

void pwmInit() {
    uint            slice;

    gpio_set_function(PWM_ANEMOMETER_PIN, GPIO_FUNC_PWM);

    // Find out which PWM slice is connected to GPIO 0 (it's slice 0)
    slice = pwm_gpio_to_slice_num(PWM_ANEMOMETER_PIN);

    // Set period of 4 cycles (0 to 3 inclusive)
    pwm_set_wrap(slice, 10849);
    
    // Set channel A output high for one cycle before dropping
    pwm_set_gpio_level(PWM_ANEMOMETER_PIN, 128);
    
    // Set the PWM running
    pwm_set_enabled(slice, true);
}
