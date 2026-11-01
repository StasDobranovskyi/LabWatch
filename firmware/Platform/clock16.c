#include "clock16.h"
void clock16_start(LwClock16 *c,uint16_t low,uint32_t ms) {
    *c=(LwClock16){0,ms,0,low};
}
uint32_t clock16_sample(LwClock16 *c,uint16_t low,uint32_t ms) {
    /* Raw counter subtraction has no dependency on UIF/IRQ delivery order.
     * At least one sample per65.536ms is required. The coarse HAL clock can
     * flag a gap, but cannot reconstruct missing wraps during flash stalls. */
    uint32_t coarse=ms-c->previous_ms;
    if(coarse>=65u)++c->gap_warnings;
    c->us+=(uint16_t)(low-c->low);c->low=low;c->previous_ms=ms;
    return c->us;
}
