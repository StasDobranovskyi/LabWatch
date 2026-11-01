#include "perf.h"
void perf_add(LwStat *s,uint32_t us) {
    if(!s->count||us<s->min_us)s->min_us=us;
    if(!s->count||us>s->max_us)s->max_us=us;
    ++s->count;s->sum_us+=us;
}
void perf_release(LwStat *s,uint32_t now,uint32_t period,uint32_t missed) {
    if(s->seen){
        int32_t jitter=(int32_t)(now-s->previous)-(int32_t)period;
        if(s->count<2||jitter<s->jitter_min)s->jitter_min=jitter;
        if(s->count<2||jitter>s->jitter_max)s->jitter_max=jitter;
    }
    s->seen=true;s->previous=now;s->missed+=missed;
}
bool perf_due(uint32_t now,uint32_t *release,uint32_t period,uint32_t *skipped) {
    if((int32_t)(now-*release)<0)return false;
    uint32_t periods=(now-*release)/period+1;
    *skipped=periods-1;*release+=periods*period;return true;
}
