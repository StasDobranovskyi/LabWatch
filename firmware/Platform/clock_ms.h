#ifndef LABWATCH_CLOCK_MS_H
#define LABWATCH_CLOCK_MS_H
#include <stdint.h>

/* Stateless conversion: arithmetic wraps modulo 2^32 microseconds. */
static inline uint32_t clock_ms_to_us(uint32_t tick_ms) {
    return tick_ms * 1000u;
}
#endif
