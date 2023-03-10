#include <stdint.h>

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/irq.h"
#include "scheduler.h"

#include "rtc_rp2040.h"

#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0

#define RTC_INTERRUPT_CYCLE             250U

void rtcDelay(uint32_t delay_us) {
    uint32_t            startTime;
    uint32_t            endTime;

    startTime = timer_hw->timerawl;
    endTime = startTime + delay_us;

    // Delay 15us...
    while (timer_hw->timerawl < endTime);
}

static void irqTimerAlarm() {
    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);

	_rtcISR();

    // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    timer_hw->alarm[ALARM_NUM] = (uint32_t)(timer_hw->timerawl + RTC_INTERRUPT_CYCLE);
}

void setupRTC() {
    // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
    // Set irq handler for alarm irq
    irq_set_exclusive_handler(TIMER_IRQ_0, irqTimerAlarm);
    // Enable the alarm irq
    irq_set_enabled(ALARM_IRQ, true);
    // Enable interrupt in block and at processor

    // Write the lower 32 bits of the target time to the alarm which
    // will arm it
    timer_hw->alarm[ALARM_NUM] = (uint32_t)(timer_hw->timerawl + RTC_INTERRUPT_CYCLE);
}
