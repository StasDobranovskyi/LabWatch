#ifndef LABWATCH_PERF_H
#define LABWATCH_PERF_H
#include <stdbool.h>
#include <stdint.h>
typedef struct {
    uint32_t count,min_us,max_us,missed,deadline_misses,previous;
    uint64_t sum_us;
    int32_t jitter_min,jitter_max;
    bool seen;
} LwStat;
void perf_add(LwStat *,uint32_t);
void perf_release(LwStat *,uint32_t,uint32_t,uint32_t);
bool perf_due(uint32_t,uint32_t *,uint32_t,uint32_t *);
#endif
