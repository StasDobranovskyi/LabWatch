#include "support.h"
#include <stdio.h>
int main(void) {
    Instrument s;Trace t={0};instrument_init(&s,capture,&t);light_ready(&s,0);
    uint32_t now=reference(&s,250,false);ImuSample p=upright();
    for(unsigned i=0;i<40;++i) {
        instrument_pir(&s,true,now);instrument_pir(&s,true,now+1);instrument_pir(&s,false,now+2);
        p.accel_mg[0]=1200;instrument_imu(&s,&p,now+3);instrument_imu(&s,&p,now+4);
        p=upright();instrument_imu(&s,&p,now+5);now+=40;
    }
    assert(events(&t,LW_OPERATOR)==40&&events(&t,LW_SHOCK)==40&&s.alarm);
    for(unsigned i=0;i<t.count;++i)if(t.rows[i].event==LW_OPERATOR)assert(t.rows[i].level==LW_INFO);
    p=(ImuSample){{200,0,980},{0,0,0},25000};
    instrument_imu(&s,&p,now);instrument_imu(&s,&p,now+499);assert(events(&t,LW_ORIENTATION)==0);
    instrument_imu(&s,&p,now+500);assert(events(&t,LW_ORIENTATION)==1);
    p=upright();instrument_imu(&s,&p,now+=520);
    for(unsigned i=0;i<10;++i)instrument_adc(&s,2200,now+=50);
    assert(s.cover&&events(&t,LW_COVER)==1);
    for(unsigned i=0;i<15;++i)instrument_adc(&s,1300,now+=50);
    assert(s.cover); /* lower threshold is inclusive */
    for(unsigned i=0;i<10;++i)instrument_adc(&s,1000,now+=50);
    assert(!s.cover&&events(&t,LW_COVER_NORMAL)==1);
    assert(!instrument_command(&s,"ack",now));assert(instrument_command(&s,"ack",now+2100));assert(!s.alarm);
    assert(instrument_command(&s,"mode service",now+=2200));
    p.accel_mg[0]=1600;instrument_imu(&s,&p,++now);
    assert(t.rows[t.count-1].event==LW_SHOCK&&t.rows[t.count-1].level==LW_INFO&&!s.alarm);
    assert(instrument_command(&s,"mode idle",++now));
    assert(!s.reference[0].valid&&!s.reference[1].valid);
    p=upright();instrument_imu(&s,&p,++now);assert(t.rows[t.count-1].event==LW_CAL_REQUIRED);
    puts("events: 40 distinct PIR/shock pairs, orientation 500ms, cover hysteresis, quiet ACK and service invalidation passed");return 0;
}
