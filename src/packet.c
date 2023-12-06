#include <stdint.h>
#include <string.h>

#include "hardware/address_mapped.h"
#include "hardware/regs/tbman.h"
#include "hardware/regs/sysinfo.h"

#include "packet.h"

const size_t PACKET_SIZE_WEATHER             = sizeof(weather_packet_t);
const size_t PACKET_SIZE_SLEEP               = sizeof(sleep_packet_t);
const size_t PACKET_SIZE_WATCHDOG            = sizeof(watchdog_packet_t);

static inline uint16_t getChipID(void) {
    return ((* ((io_ro_32 *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET))) >> 12) & 0xFFFF;
}

weather_packet_t * getWeatherPacket(void) {
    static weather_packet_t     weather;

    weather.packetID = PACKET_ID_WEATHER;
    
    return &weather;
}

sleep_packet_t * getSleepPacket(void) {
    static sleep_packet_t     sleep;

    sleep.packetID = PACKET_ID_SLEEP;
    
    return &sleep;
}

watchdog_packet_t * getWatchdogPacket(void) {
    static watchdog_packet_t     wp;

    wp.packetID = PACKET_ID_WEATHER;
    
    return &wp;
}
