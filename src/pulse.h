// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ----- //
// pulse //
// ----- //

#define pulse_wrap_target 0
#define pulse_wrap 2

static const uint16_t pulse_program_instructions[] = {
            //     .wrap_target
    0x2030, //  0: wait   0 pin, 16                  
    0x20b0, //  1: wait   1 pin, 16                  
    0x0040, //  2: jmp    x--, 0                     
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program pulse_program = {
    .instructions = pulse_program_instructions,
    .length = 3,
    .origin = -1,
};

static inline pio_sm_config pulse_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + pulse_wrap_target, offset + pulse_wrap);
    return c;
}
#endif

