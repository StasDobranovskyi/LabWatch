#include "runtime.h"
#include "board.h"
#include "main.h"
#include "instrument.h"
#include "perf.h"
#include "build_id.h"
#include "logfmt.h"
#include <string.h>
#ifdef RUNTIME_TEST_PRINTF
int runtime_test_printf(const char *, ...);
#define report runtime_test_printf
#else
#define report lw_printf
#endif

enum { TASK_RX, TASK_PIR, TASK_RETRY, TASK_IMU, TASK_ADC, TASK_JOURNAL, TASK_TELEMETRY, TASK_TASKS };
static const char *const names[]={"rx","pir","retry","imu","adc","journal","telemetry"};
static const uint32_t periods[]={1000,1000,1000000,20000,50000,20000,1000000};
static const uint32_t deadlines[]={100000,100000,1000000,20000,50000,100000,1000000};
static uint32_t releases[TASK_TASKS];
static LwStat stats[TASK_TASKS],loop_stat,pir_latency,command_latency;
static Mpu6050 imu;static Instrument instrument;static LwJournal journal;
static bool imu_ready,storage_ready,storage_fault;
static LwRecord pending[16];static unsigned queued,queue_read;
static uint32_t queue_lost,tx_lost,storage_errors,stored,rx_lost;
static uint32_t busy_us,load_since,load_permille;
static uint32_t boot_us,first_imu,first_adc,first_pir;
static char command[80];static unsigned used;static bool rejected;
static unsigned dump_slot,stats_page;static bool dumping,printing_stats;

static void print_record(const char *prefix,const LwRecord *r) {
    report("%s seq=%lu time_ms=%lu mode=%s src=%s event=%s level=%s value=%ld detail=%ld\r\n",prefix,
      (unsigned long)r->sequence,(unsigned long)r->time_ms,instrument_mode(r->mode),
      instrument_source(r->event),instrument_event(r->event),instrument_level(r->level),(long)r->value,(long)r->detail);
}
static bool durable(LwEvent e) { return e!=LW_STATUS&&e!=LW_BAD_COMMAND&&e!=LW_BAD_LINE&&e!=LW_RX_LOSS&&e!=LW_DENIED; }
static void event(void *ctx,const LwRecord *record) {
    (void)ctx;print_record("EVT",record);
    if(!durable(record->event))return;
    if(queued==16){++queue_lost;return;}
    pending[(queue_read+queued)%16]=*record;++queued;
}
static void note(LwEvent e,LwLevel l,int32_t v,int32_t d) {
    instrument_note(&instrument,e,l,v,d,HAL_GetTick());
}
static void print_stat(const char *name,const LwStat *s) {
    report("STAT name=%s n=%lu min_us=%lu avg_us=%lu max_us=%lu jitter_min_us=%ld jitter_max_us=%ld skipped=%lu deadline_misses=%lu\r\n",
     name,(unsigned long)s->count,(unsigned long)s->min_us,
     (unsigned long)(s->count?s->sum_us/s->count:0),(unsigned long)s->max_us,
     (long)s->jitter_min,(long)s->jitter_max,(unsigned long)s->missed,(unsigned long)s->deadline_misses);
}
static void execute(uint32_t completed) {
    perf_add(&command_latency,board_micros()-completed);
    if(!strcmp(command,"rxdiag")) {
        LwRxDiagnostic d=board_rx_diagnostic();
        report("RXDIAG attempts=%lu failures=%lu last=%d pending=%u state=%lu error=%lu irq=%u ore=%u\r\n",
          (unsigned long)d.attempts,(unsigned long)d.failures,d.last_status,d.pending,
          (unsigned long)d.hal_state,(unsigned long)d.hal_error,d.irq_enabled,d.overrun);
        return;
    }
    if(!strcmp(command,"storage")) {
        LwFlashDiagnostic d=board_storage_diagnostic();
        report("FLASH erases=%lu rejected=%lu bad_offset=%lu bad_word=%lu\r\n",
          (unsigned long)d.erases,(unsigned long)d.erase_rejected,(unsigned long)d.first_bad_offset,(unsigned long)d.first_bad_word);
        return;
    }
    if(!strcmp(command,"version")){report("VERSION source=%s base=%s\r\n",BUILD_ID,GIT_BASE);return;}
    if(!strcmp(command,"stats")){printing_stats=true;stats_page=0;return;}
    if(!strcmp(command,"journal dump")){dumping=true;dump_slot=0;return;}
    if(!strcmp(command,"mark")){note(LW_CHECKPOINT,LW_INFO,0,0);return;}
    if(!strcmp(command,"imuaddr 0x68")||!strcmp(command,"imuaddr 0x69")) {
        imu.address=command[11]=='8'?0x68:0x69;imu_ready=false;
        instrument_fault(&instrument,true,true,HAL_GetTick());releases[TASK_RETRY]=board_micros();return;
    }
    if(!strcmp(command,"selftest")) {
        ImuSample sample;BusResult result=mpu6050_init(&imu);
        if(result==BUS_OK)result=mpu6050_read(&imu,&sample);
        note(LW_SELFTEST,result==BUS_OK?LW_INFO:LW_ERROR,result,0);
        if(result!=BUS_OK){imu_ready=false;instrument_fault(&instrument,true,true,HAL_GetTick());}
        return;
    }
    (void)instrument_command(&instrument,command,HAL_GetTick());
}
static void receive(void) {
    board_rx_service();
    if(board_rx_overflow()){++rx_lost;rejected=true;used=0;note(LW_RX_LOSS,LW_ERROR,0,0);}
    for(unsigned bytes=0;bytes<64;++bytes) {
        uint32_t time;int ch=board_get_char_at(&time);if(ch<0)return;
        if(ch=='\n'||ch=='\r') {
            if(rejected)note(LW_BAD_LINE,LW_ERROR,0,0);
            else if(used){command[used]=0;execute(time);}
            used=0;rejected=false;return; /* At most one command per dispatch. */
        }
        if(ch<32||ch>126||used==sizeof(command)-1){used=0;rejected=true;}
        else if(!rejected)command[used++]=(char)ch;
    }
}
static void sample_pir(void) {
    bool high;uint32_t time;
    for(unsigned count=0;count<7&&board_pir_event(&high,&time);++count) {
        board_probe(2,true);
        if(high)perf_add(&pir_latency,board_micros()-time);
        instrument_pir(&instrument,high,HAL_GetTick());board_probe(2,false);
    }
    /* Reconcile current level after queue overflow; lost pulses remain explicitly counted. */
    instrument_pir(&instrument,board_motion(),HAL_GetTick());
    if(!first_pir)first_pir=board_micros()-boot_us+1;
}
static void service_journal(void) {
    if(storage_ready&&queued) {
        board_probe(1,true);
        if(journal_append(&journal,&pending[queue_read])){
            queue_read=(queue_read+1)%16;--queued;++stored;storage_fault=false;
        } else {++storage_errors;storage_fault=true;}
        board_probe(1,false);
    }
    if(dumping) {
        LwRecord r;
        if(storage_ready&&journal_record(&journal,dump_slot,&r))print_record("JREC",&r);
        if(++dump_slot==LW_JOURNAL_SLOTS)dumping=false;
    }
    if(printing_stats) {
        if(stats_page<TASK_TASKS)print_stat(names[stats_page],&stats[stats_page]);
        else if(stats_page==TASK_TASKS)print_stat("loop",&loop_stat);
        else if(stats_page==TASK_TASKS+1)print_stat("pir_irq_to_dispatch",&pir_latency);
        else if(stats_page==TASK_TASKS+2)print_stat("command_rx_to_dispatch",&command_latency);
        else {
            report("MEM stack_used=%lu stack_reserve=2048 heap_peak=%lu\r\n",(unsigned long)board_stack_used(),(unsigned long)board_heap_peak());
            printing_stats=false;
        }
        ++stats_page;
    }
}
static void run_task(unsigned task) {
    uint32_t ms=HAL_GetTick();
    switch(task) {
    case TASK_RX:receive();break;
    case TASK_PIR:sample_pir();break;
    case TASK_RETRY:
        if(!imu_ready){imu_ready=mpu6050_init(&imu)==BUS_OK;instrument_fault(&instrument,true,!imu_ready,HAL_GetTick());}
        break;
    case TASK_IMU:
        if(imu_ready){
            ImuSample sample;board_probe(0,true);
            bool ok=mpu6050_read(&imu,&sample)==BUS_OK;
            if(ok){instrument_imu(&instrument,&sample,HAL_GetTick());if(!first_imu)first_imu=board_micros()-boot_us+1;}
            else {imu_ready=false;instrument_fault(&instrument,true,true,HAL_GetTick());}
            board_probe(0,false);
        }
        break;
    case TASK_ADC: {
        /* TASK_ADC failure is retried at 1 Hz rather than every 50 ms. */
        static uint32_t adc_retry;
        if(instrument.adc_fault&&(uint32_t)(ms-adc_retry)<1000)break;
        uint16_t light;bool ok=board_read_light(&light);adc_retry=ms;
        instrument_fault(&instrument,false,!ok,HAL_GetTick());
        if(ok){instrument_adc(&instrument,light,HAL_GetTick());if(!first_adc)first_adc=board_micros()-boot_us+1;}
        break;
    }
    case TASK_JOURNAL:service_journal();break;
    case TASK_TELEMETRY:
        report("SAMPLE time_us=%lu ax_mg=%ld ay_mg=%ld az_mg=%ld gx_mdps=%ld gy_mdps=%ld gz_mdps=%ld light_raw=%u\r\n",
          (unsigned long)board_micros(),(long)instrument.latest.accel_mg[0],(long)instrument.latest.accel_mg[1],
          (long)instrument.latest.accel_mg[2],(long)instrument.latest.gyro_mdps[0],(long)instrument.latest.gyro_mdps[1],
          (long)instrument.latest.gyro_mdps[2],instrument.light);
        LwRxDiagnostic rd=board_rx_diagnostic();
        report("HEALTH time_ms=%lu mode=%s imu=%u adc=%u base=%u,%u cal=%u light=%u alarm=%u stored=%lu queued=%u queue_lost=%lu rx_loss=%lu tx_loss=%lu pir_lost=%lu storage_fault=%u storage_errors=%lu durable_seq=%lu crc_bad=%lu load_estimate_permille=%lu clock_resolution_us=1000 clock_gap_monitor=unavailable init_imu_us=%lu init_adc_us=%lu init_pir_us=%lu\r\n",
          (unsigned long)ms,instrument_mode(instrument.mode),imu_ready,!instrument.adc_fault,
          instrument.reference[0].valid,instrument.reference[1].valid,instrument.calibrating,instrument.light,instrument.alarm,
          (unsigned long)stored,queued,(unsigned long)queue_lost,(unsigned long)rx_lost,(unsigned long)tx_lost,
          (unsigned long)board_pir_lost(),storage_fault,(unsigned long)storage_errors,
          (unsigned long)journal.last_sequence,(unsigned long)journal.damaged,(unsigned long)load_permille,
          (unsigned long)first_imu,(unsigned long)first_adc,(unsigned long)first_pir);
        report("RXHEALTH time_ms=%lu failures=%lu pending=%u state=%lu irq=%u ore=%u\r\n",
          (unsigned long)ms,(unsigned long)rd.failures,rd.pending,(unsigned long)rd.hal_state,rd.irq_enabled,rd.overrun);
        break;
    }
}
void runtime_init(void) {
    board_clock_init();boot_us=board_micros();board_init();
    imu=(Mpu6050){board_sensor_bus(),0x68};
    instrument_init(&instrument,event,NULL);
    storage_ready=journal_open(&journal,board_storage());storage_fault=!storage_ready;
    if(storage_ready)instrument.sequence=journal.last_sequence;
    uint32_t t=board_micros();for(unsigned i=0;i<TASK_TASKS;++i)releases[i]=t;
    load_since=t;
    report("BOOT product=LabWatch base=%s source=%s clock_hz=48000000 timer=TIM3_1MHz sample=us image=labwatch-build:lab2\r\n",GIT_BASE,BUILD_ID);
    note(LW_BOOT,LW_INFO,storage_ready,journal.damaged);
}
void runtime_poll(void) {
    uint32_t start=board_micros();
    for(unsigned i=0;i<TASK_TASKS;++i) {
        uint32_t t=board_micros(),missed,release=releases[i];
        if(!perf_due(t,&releases[i],periods[i],&missed))continue;
        perf_release(&stats[i],t,periods[i],missed);run_task(i);
        uint32_t end=board_micros();perf_add(&stats[i],end-t);busy_us+=end-t;
        if((uint32_t)(end-release)>deadlines[i])++stats[i].deadline_misses;
        /* A failed physical flash write gets a bounded retry rate. */
        if(i==TASK_JOURNAL&&storage_fault)releases[i]=end+1000000u;
    }
    uint32_t end=board_micros();perf_add(&loop_stat,end-start);
    bool fault=instrument.imu_fault||instrument.adc_fault||storage_fault;
    uint32_t ms=HAL_GetTick();board_led(fault?(ms/200)%2:instrument.alarm||(ms/1000)%2);
    if(board_tx_overflow())++tx_lost;
    uint32_t window=end-load_since;
    if(window>=1000000){load_permille=(uint32_t)((uint64_t)busy_us*1000/window);busy_us=0;load_since=end;}
}
