#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "hardware/rtc.h"
#include "scheduler.h"

#include "rtc_rp2040.h"

#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0

#define RTC_INTERRUPT_CYCLE             1000U

datetime_t * _fillDateTime(datetime_t * dt) {
    dt->day         = 14;
    dt->dotw        = 5;
    dt->month       = 4;
    dt->year        = 2022;

    dt->hour        = 14;
    dt->min         = 20;
    dt->sec         = 0;

    return dt;
}

void rtcDelay(uint32_t delay_us) {
    uint32_t            startTime;
    uint32_t            endTime;

    startTime = timer_hw->timerawl;
    endTime = startTime + delay_us;

    while (timer_hw->timerawl < endTime);
}

static void irqTimerAlarm() {
    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

	_rtcISR();

    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    timer_hw->alarm[ALARM_NUM] = (uint32_t)(timer_hw->timerawl + RTC_INTERRUPT_CYCLE);
}

void setupRTC() {
    // datetime_t          dt;

    // rtc_init();

    // rtc_set_datetime(_fillDateTime(&dt));

    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);

    irq_set_exclusive_handler(TIMER_IRQ_0, irqTimerAlarm);
    irq_set_enabled(ALARM_IRQ, true);

    timer_hw->alarm[ALARM_NUM] = (uint32_t)(timer_hw->timerawl + RTC_INTERRUPT_CYCLE);
}
