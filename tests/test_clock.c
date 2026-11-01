#include "clock16.h"
#include "clock_ms.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    LwClock16 c;clock16_start(&c,0,0);
    for(uint32_t t=1;t<=10000000u;t+=113u)assert(clock16_sample(&c,(uint16_t)t,t/1000)==t);
    assert(!c.gap_warnings);
    clock16_start(&c,65530,1000);assert(clock16_sample(&c,2,1000)==8);
    c.us=UINT32_MAX-7;assert(clock16_sample(&c,10,1001)==0);
    clock16_start(&c,0,UINT32_MAX-10);assert(clock16_sample(&c,20000,9)==20000);
    assert(!c.gap_warnings);
    clock16_start(&c,0,0);(void)clock16_sample(&c,4464,70);assert(c.gap_warnings==1);
    /* The outer read is interrupted by an inner read. Stateless conversion
       cannot corrupt either snapshot, unlike a shared 16-bit epoch. */
    uint32_t outer_tick=4294967u;
    uint32_t inner_tick=outer_tick+1u;
    uint32_t inner=clock_ms_to_us(inner_tick);
    uint32_t outer=clock_ms_to_us(outer_tick);
    assert((uint32_t)(inner-outer)==1000u);
    assert((uint32_t)(clock_ms_to_us(0)-clock_ms_to_us(UINT32_MAX))==1000u);
    assert(clock_ms_to_us(42u)==42000u);
    puts("clock: stateless 1ms conversion, interrupted outer read and 32-bit rollover passed");return 0;
}
