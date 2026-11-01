#include "instrument.h"
#include <limits.h>
#include <math.h>
#include <string.h>

static int32_t length(const int32_t v[3]) {
    return (int32_t)sqrtf((float)v[0]*v[0]+(float)v[1]*v[1]+(float)v[2]*v[2]);
}
const char *instrument_mode(LwMode m) {
    static const char *const n[]={"idle","work","service"};
    return m<=LW_SERVICE?n[m]:"invalid";
}
const char *instrument_level(LwLevel l) {
    static const char *const n[]={"INFO","ALARM","ERROR"};
    return l<=LW_ERROR?n[l]:"invalid";
}
const char *instrument_event(LwEvent e) {
    static const char *const n[]={"invalid","mode_change","operator_motion","shock_candidate",
      "instrument_motion","light_change","light_normal","calibrating","calibration_ready",
      "calibration_rejected","calibration_required","calibration_interrupted","imu_error",
      "imu_recovered","adc_error","adc_recovered","acknowledged","denied","status",
      "command_error","rx_overflow","invalid_line","selftest","boot","checkpoint"};
    return e<=LW_CHECKPOINT?n[e]:"invalid";
}
const char *instrument_source(LwEvent e) {
    if(e==LW_OPERATOR)return "GPIO";
    if(e==LW_SHOCK||e==LW_ORIENTATION||e==LW_IMU_FAULT||e==LW_IMU_RECOVER||e==LW_SELFTEST)return "IMU";
    if(e==LW_COVER||e==LW_COVER_NORMAL||e==LW_ADC_FAULT||e==LW_ADC_RECOVER)return "ADC";
    if(e>=LW_CAL_START&&e<=LW_CAL_ABORT)return "CAL";
    if(e==LW_BOOT||e==LW_CHECKPOINT)return "SYS";
    return "CMD";
}
void instrument_note(Instrument *s,LwEvent e,LwLevel l,int32_t v,int32_t d,uint32_t t) {
    if(l==LW_ALARM)s->alarm=true;
    if(s->sequence==UINT32_MAX)return; /* Sequence exhaustion is fail-closed. */
    LwRecord r={++s->sequence,t,e,s->mode,l,v,d};
    if(s->sink)s->sink(s->sink_context,&r);
}
void instrument_init(Instrument *s,LwSink sink,void *ctx) {
    memset(s,0,sizeof *s); s->sink=sink; s->sink_context=ctx;
}
static LwLevel mechanical(const Instrument *s) { return s->mode==LW_SERVICE?LW_INFO:LW_ALARM; }
static unsigned slot(const Instrument *s) { return s->mode==LW_WORK?1:0; }
static void reset_candidates(Instrument *s,uint32_t t) {
    s->shock=false;s->tilted=false;s->tilt_wait=false;s->cover=false;s->cover_wait=false;s->quiet_since=t;
}
void instrument_fault(Instrument *s,bool imu,bool fault,uint32_t t) {
    bool *p=imu?&s->imu_fault:&s->adc_fault;
    if(*p==fault)return;
    *p=fault;
    instrument_note(s,imu?(fault?LW_IMU_FAULT:LW_IMU_RECOVER):(fault?LW_ADC_FAULT:LW_ADC_RECOVER),
                    fault?LW_ERROR:LW_INFO,0,0,t);
    if(fault) {
        s->reference[0].valid=s->reference[1].valid=false;s->service_reference.valid=false;
        s->notice[0]=s->notice[1]=false;reset_candidates(s,t);
        if(s->calibrating){s->calibrating=false;instrument_note(s,LW_CAL_ABORT,LW_ERROR,imu,0,t);}
        if(!imu){s->light_count=0;s->light_position=0;}
    }
}
void instrument_pir(Instrument *s,bool high,uint32_t t) {
    if(high&&!s->pir)instrument_note(s,LW_OPERATOR,LW_INFO,1,0,t);
    s->pir=high;
}
static void calibrate_sample(Instrument *s,const ImuSample *p,uint32_t t) {
    for(unsigned a=0;a<3;++a) {
        int32_t v=p->accel_mg[a];s->cal_sum[a]+=v;
        if(v<s->cal_low[a])s->cal_low[a]=v;
        if(v>s->cal_high[a])s->cal_high[a]=v;
    }
    int32_t gyro=length(p->gyro_mdps);
    if(gyro>s->cal_rotation)s->cal_rotation=gyro;
    if(++s->cal_samples<500)return;
    s->calibrating=false;
    int32_t spread[3];
    for(unsigned a=0;a<3;++a)spread[a]=s->cal_high[a]-s->cal_low[a];
    int32_t envelope=length(spread);
    unsigned k=slot(s);
    /* The diagonal of the observed box bounds every sample's distance from its mean. */
    int32_t allowed=s->mode==LW_WORK?500:50;
    bool ok=envelope<=allowed&&s->cal_rotation<=3000&&s->light_count==5&&
            !s->imu_fault&&!s->adc_fault&&(uint32_t)(t-s->cal_started)>=9980u;
    s->reference[k].valid=ok;
    if(ok){
        for(unsigned a=0;a<3;++a)s->reference[k].center[a]=s->cal_sum[a]/500;
        s->reference[k].envelope=envelope;s->cover_base[k]=s->light;reset_candidates(s,t);
    }
    instrument_note(s,ok?LW_CAL_READY:LW_CAL_REJECT,ok?LW_INFO:LW_ERROR,envelope,k,t);
}
void instrument_imu(Instrument *s,const ImuSample *p,uint32_t t) {
    s->latest=*p;
    if(s->imu_fault)return;
    if(s->calibrating){calibrate_sample(s,p,t);return;}
    unsigned k=slot(s);const LwReference *r=s->mode==LW_SERVICE?&s->service_reference:&s->reference[k];
    if(!r->valid){
        if(!s->notice[k]){s->notice[k]=true;instrument_note(s,LW_CAL_REQUIRED,LW_INFO,k,0,t);}
        return;
    }
    int32_t delta[3];
    for(unsigned a=0;a<3;++a)delta[a]=p->accel_mg[a]-r->center[a];
    int32_t deviation=length(delta),limit=r->envelope+800;
    if(!s->shock&&deviation>limit){s->shock=true;instrument_note(s,LW_SHOCK,mechanical(s),deviation,limit,t);}
    if(deviation<limit/2)s->shock=false;
    /* Normalise before comparing angles: avoids overflowing a product of squared raw vectors. */
    float dot=0,aa=0,bb=0;
    for(unsigned a=0;a<3;++a){float x=p->accel_mg[a],y=r->center[a];dot+=x*y;aa+=x*x;bb+=y*y;}
    bool tilted=aa>250000&&bb>250000&&dot/sqrtf(aa*bb)<0.98480775f;
    if(tilted) {
        if(!s->tilt_wait){s->tilt_wait=true;s->tilt_since=t;}
        if(!s->tilted&&(uint32_t)(t-s->tilt_since)>=500){
            s->tilted=true;instrument_note(s,LW_ORIENTATION,mechanical(s),deviation,500,t);
        }
    } else {s->tilt_wait=false;s->tilted=false;}
    if(s->shock||s->tilt_wait)s->quiet_since=t;
}
void instrument_adc(Instrument *s,uint16_t raw,uint32_t t) {
    if(s->adc_fault||raw>4095)return;
    s->light_samples[s->light_position]=raw;s->light_position=(s->light_position+1)%5;
    if(s->light_count<5)++s->light_count;
    uint32_t sum=0;for(unsigned i=0;i<s->light_count;++i)sum+=s->light_samples[i];
    s->light=(uint16_t)(sum/s->light_count);
    unsigned k=slot(s);
    if(s->light_count<5||s->calibrating||!(s->mode==LW_SERVICE?s->service_reference.valid:s->reference[k].valid))return;
    int32_t d=(int32_t)s->light-(s->mode==LW_SERVICE?s->service_cover:s->cover_base[k]);if(d<0)d=-d;
    bool changed=d>=(s->cover?300:500);
    if(changed) {
        if(!s->cover_wait){s->cover_wait=true;s->cover_since=t;}
        if(!s->cover&&(uint32_t)(t-s->cover_since)>=200){
            s->cover=true;instrument_note(s,LW_COVER,mechanical(s),s->light,d,t);
        }
        s->quiet_since=t;
    } else {
        if(s->cover)instrument_note(s,LW_COVER_NORMAL,LW_INFO,s->light,d,t);
        s->cover_wait=false;s->cover=false;
    }
}
bool instrument_command(Instrument *s,const char *line,uint32_t t) {
    if(!strcmp(line,"status")){instrument_note(s,LW_STATUS,LW_INFO,s->alarm,s->cal_samples,t);return true;}
    if(!strcmp(line,"calibrate")) {
        if(s->mode==LW_SERVICE||s->imu_fault||s->adc_fault||s->calibrating)goto denied;
        s->calibrating=true;s->cal_samples=0;s->cal_rotation=0;s->cal_started=t;
        s->reference[slot(s)].valid=false;
        for(unsigned a=0;a<3;++a){s->cal_sum[a]=0;s->cal_low[a]=INT32_MAX;s->cal_high[a]=INT32_MIN;}
        instrument_note(s,LW_CAL_START,LW_INFO,slot(s),0,t);return true;
    }
    if(!strcmp(line,"ack")) {
        if(s->imu_fault||s->adc_fault||s->calibrating||!s->reference[slot(s)].valid||
           s->shock||s->tilt_wait||s->cover_wait||(uint32_t)(t-s->quiet_since)<2000)goto denied;
        s->alarm=false;instrument_note(s,LW_ACK,LW_INFO,0,0,t);return true;
    }
    const char *const modes[]={"mode idle","mode work","mode service"};
    for(unsigned m=0;m<3;++m)if(!strcmp(line,modes[m])) {
        LwMode old=s->mode;
        if(m==(unsigned)old)return true;
        if(s->calibrating){s->calibrating=false;instrument_note(s,LW_CAL_ABORT,LW_ERROR,0,0,t);}
        if(old==LW_SERVICE){s->reference[0].valid=s->reference[1].valid=false;s->notice[0]=s->notice[1]=false;}
        if(m==LW_SERVICE) {
            s->service_reference.valid=!s->imu_fault&&s->light_count==5;
            for(unsigned a=0;a<3;++a)s->service_reference.center[a]=s->latest.accel_mg[a];
            s->service_reference.envelope=0;s->service_cover=s->light;
        }
        s->mode=(LwMode)m;reset_candidates(s,t);
        instrument_note(s,LW_MODE,LW_INFO,old,m,t);return true;
    }
    instrument_note(s,LW_BAD_COMMAND,LW_ERROR,0,0,t);return false;
denied:
    instrument_note(s,LW_DENIED,LW_ERROR,0,0,t);return false;
}
