#include "support.h"
#include <stdio.h>
int main(void) {
    Instrument s;Trace t={0};instrument_init(&s,capture,&t);light_ready(&s,0);
    uint32_t now=reference(&s,250,false);
    assert(s.reference[0].center[2]==1000&&s.reference[0].envelope==0&&!s.reference[1].valid);
    assert(instrument_command(&s,"mode work",++now));now=reference(&s,now,true);
    assert(s.reference[0].valid&&s.reference[1].valid&&s.reference[1].envelope==300);
    for(unsigned i=0;i<2000;++i){ImuSample p=upright();p.accel_mg[0]=(i%2?150:-150);instrument_imu(&s,&p,now+=20);}
    assert(!events(&t,LW_SHOCK));
    assert(instrument_command(&s,"calibrate",now));
    for(unsigned i=0;i<500;++i){ImuSample p=upright();p.accel_mg[0]=i%2?500:-500;instrument_imu(&s,&p,now+=20);}
    assert(!s.reference[1].valid&&events(&t,LW_CAL_REJECT)==1);
    assert(instrument_command(&s,"calibrate",now));instrument_fault(&s,true,true,++now);
    assert(!s.calibrating&&!s.reference[0].valid);assert(events(&t,LW_CAL_ABORT)==1);
    unsigned n=t.count;instrument_fault(&s,true,true,++now);assert(t.count==n);
    instrument_fault(&s,true,false,++now);assert(!s.reference[1].valid);
    instrument_init(&s,NULL,NULL);light_ready(&s,0);
    now=reference(&s,UINT32_MAX-5000,false);assert(s.reference[0].valid&&now<10000);
    assert(!instrument_command(&s,"mode guard",now)&&s.mode==LW_IDLE);
    puts("calibration: separate idle/work references, 2000 background samples, rejection, interruption and ms rollover passed");return 0;
}
