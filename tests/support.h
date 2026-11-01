#ifndef LABWATCH_TEST_SUPPORT_H
#define LABWATCH_TEST_SUPPORT_H
#include "instrument.h"
#include <assert.h>
#include <string.h>
typedef struct { LwRecord rows[4096];unsigned count; } Trace;
static inline void capture(void *ctx,const LwRecord *r) {
    Trace *t=ctx;assert(t->count<4096);t->rows[t->count++]=*r;
}
static inline unsigned events(const Trace *t,LwEvent e) {
    unsigned n=0;for(unsigned i=0;i<t->count;++i)n+=t->rows[i].event==e;return n;
}
static inline ImuSample upright(void) { return (ImuSample){{0,0,1000},{0,0,0},25000}; }
static inline void light_ready(Instrument *s,uint32_t t) {
    for(unsigned i=0;i<5;++i)instrument_adc(s,1000,t+i*50);
}
static inline uint32_t reference(Instrument *s,uint32_t t,bool vibrate) {
    assert(instrument_command(s,"calibrate",t));
    for(unsigned n=1;n<=500;++n) {
        ImuSample p=upright();p.accel_mg[0]=vibrate?(n%2?150:-150):0;
        instrument_adc(s,1000,t+n*20);instrument_imu(s,&p,t+n*20);
    }
    assert(!s->calibrating&&s->reference[s->mode==LW_WORK].valid);return t+10000;
}
#endif
