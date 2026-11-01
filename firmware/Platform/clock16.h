#ifndef LABWATCH_CLOCK16_H
#define LABWATCH_CLOCK16_H
#include <stdint.h>
typedef struct { uint32_t us,previous_ms,gap_warnings;uint16_t low; } LwClock16;
void clock16_start(LwClock16 *,uint16_t,uint32_t);
uint32_t clock16_sample(LwClock16 *,uint16_t,uint32_t);
#endif
