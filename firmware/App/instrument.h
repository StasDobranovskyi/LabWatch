#ifndef LABWATCH_INSTRUMENT_H
#define LABWATCH_INSTRUMENT_H
#include "mpu6050.h"
#include <stdbool.h>
#include <stdint.h>
typedef enum { LW_IDLE, LW_WORK, LW_SERVICE } LwMode;
typedef enum { LW_INFO, LW_ALARM, LW_ERROR } LwLevel;
typedef enum {
    LW_MODE=1, LW_OPERATOR, LW_SHOCK, LW_ORIENTATION, LW_COVER, LW_COVER_NORMAL,
    LW_CAL_START, LW_CAL_READY, LW_CAL_REJECT, LW_CAL_REQUIRED, LW_CAL_ABORT,
    LW_IMU_FAULT, LW_IMU_RECOVER, LW_ADC_FAULT, LW_ADC_RECOVER,
    LW_ACK, LW_DENIED, LW_STATUS, LW_BAD_COMMAND, LW_RX_LOSS, LW_BAD_LINE,
    LW_SELFTEST, LW_BOOT, LW_CHECKPOINT
} LwEvent;
typedef struct {
    uint32_t sequence, time_ms;
    LwEvent event; LwMode mode; LwLevel level;
    int32_t value, detail;
} LwRecord;
typedef void (*LwSink)(void *, const LwRecord *);
typedef struct { int32_t center[3], envelope; bool valid; } LwReference;
typedef struct {
    LwMode mode; LwReference reference[2];
    LwReference service_reference; uint16_t service_cover;
    ImuSample latest;
    LwSink sink; void *sink_context; uint32_t sequence;
    bool imu_fault, adc_fault, pir, alarm;
    bool calibrating, shock, tilted, tilt_wait, cover, cover_wait, notice[2];
    uint32_t tilt_since, cover_since, quiet_since, cal_started;
    unsigned cal_samples; int32_t cal_sum[3], cal_low[3], cal_high[3];
    int32_t cal_rotation;
    uint16_t light_samples[5], light, cover_base[2];
    unsigned light_position, light_count;
} Instrument;
void instrument_init(Instrument *, LwSink, void *);
void instrument_note(Instrument *, LwEvent, LwLevel, int32_t, int32_t, uint32_t);
void instrument_imu(Instrument *, const ImuSample *, uint32_t);
void instrument_adc(Instrument *, uint16_t, uint32_t);
void instrument_pir(Instrument *, bool, uint32_t);
void instrument_fault(Instrument *, bool imu, bool fault, uint32_t);
bool instrument_command(Instrument *, const char *, uint32_t);
const char *instrument_mode(LwMode);
const char *instrument_event(LwEvent);
const char *instrument_level(LwLevel);
const char *instrument_source(LwEvent);
#endif
