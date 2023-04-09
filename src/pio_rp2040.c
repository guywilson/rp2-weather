#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hardware/pio.h"
#include "pio_rp2040.h"
#include "rtc_rp2040.h"
#include "scheduler.h"
#include "taskdef.h"
#include "sensor.h"
#include "pulse.h"

void pioInit(void) {

}

/*
** We measure wind speed in m/s. This task runs
** once per second, so adding up the number of pulses
** in the last second, and knowing the diameter of
** our anemometer, we can work out our wind speed.
*/
void taskAnemometer(PTASKPARM p) {

}

/*
** We measure rainfall in mm/h. This task runs
** once per hour, so adding up the number of pulses
** in the last hour, and knowing the volume of
** our tipping bucket, we can work out our rainfall.
*/
void taskRainGuage(PTASKPARM p) {

}
