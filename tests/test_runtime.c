#include "board.h"
#include "runtime.h"
#include "flash_fake.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
static uint64_t time_us;
static FlashFake memory;
static char input[8192],output[524288];static size_t in_count,in_read,out_count;
static bool bus_failure,adc_failure,rx_overflow,pir_level,edge_pending,edge_high;
static uint32_t edge_time;static unsigned adc_failures,probes;
static unsigned injected,erase_count;
uint32_t HAL_GetTick(void) { return (uint32_t)(time_us/1000); }
uint32_t board_clock_gaps(void) { return 0; }
void board_rx_service(void) {}
LwRxDiagnostic board_rx_diagnostic(void) { return (LwRxDiagnostic){0}; }
LwFlashDiagnostic board_storage_diagnostic(void) { return (LwFlashDiagnostic){0}; }
uint32_t board_micros(void) { return (uint32_t)time_us; }
void board_clock_init(void) {}
void board_init(void) {}
void board_probe(unsigned n,bool on) { (void)n;(void)on; }
uint32_t board_stack_used(void) { return 0; } /* Host fake: never a stack measurement. */
uint32_t board_heap_peak(void) { return 0; }
int runtime_test_printf(const char *format,...) {
    va_list args;va_start(args,format);
    int n=vsnprintf(output+out_count,sizeof output-out_count,format,args);va_end(args);
    assert(n>=0&&(size_t)n<sizeof output-out_count);out_count+=(size_t)n;return n;
}
static BusResult read_bus(void *ctx,uint8_t address,uint8_t reg,uint8_t *p,size_t n) {
    (void)ctx;time_us+=420;
    if(reg==0x75)++probes;
    if(bus_failure||address!=0x68)return BUS_NACK;
    memset(p,0,n);
    if(reg==0x75)p[0]=0x68;
    else if(reg==0x19){assert(n==4);p[0]=19;p[1]=3;}
    else if(reg==0x6b){assert(n==1);p[0]=1;}
    else {assert(n==14);p[4]=0x40;}
    return BUS_OK;
}
static BusResult write_bus(void *ctx,uint8_t a,uint8_t reg,const uint8_t *p,size_t n) {
    (void)ctx;(void)p;assert((reg==0x6b&&n==1)||(reg==0x19&&n==4));time_us+=100;return bus_failure||a!=0x68?BUS_NACK:BUS_OK;
}
SensorBus board_sensor_bus(void) { return (SensorBus){NULL,read_bus,write_bus}; }
bool board_read_light(uint16_t *p) { time_us+=65;if(adc_failure){++adc_failures;return false;}*p=1000;return true; }
bool board_motion(void) { return pir_level; }
void board_led(bool high) { (void)high; }
int board_get_char_at(uint32_t *t) { *t=(uint32_t)time_us;return in_read<in_count?(unsigned char)input[in_read++]:-1; }
int board_get_char(void) { uint32_t t;return board_get_char_at(&t); }
int board_put_char(int c) { return c; }
bool board_write_line(const char *s,size_t n) { (void)s;(void)n;return true; }
bool board_rx_overflow(void) { bool v=rx_overflow;rx_overflow=false;return v; }
bool board_tx_overflow(void) { return false; }
uint32_t board_pir_lost(void) { return 0; }
bool board_pir_event(bool *high,uint32_t *time) {
    if(!edge_pending)return false;
    *high=edge_high;*time=edge_time;edge_pending=false;return true;
}
static bool memory_read(void *ctx,unsigned at,void *to,size_t n) { return flash_read(ctx,at,to,n); }
static bool memory_program(void *ctx,unsigned at,const uint8_t p[8]) { time_us+=100;return flash_write(ctx,at,p); }
static bool memory_erase(void *ctx,unsigned page) {
    ++erase_count;
    /* Deterministic injection into a HOST duration model, not a peripheral run. */
    edge_pending=true;edge_high=true;pir_level=true;edge_time=(uint32_t)time_us+1000;++injected;
    time_us+=60000;return flash_erase(ctx,page);
}
LwStorage board_storage(void) { return (LwStorage){&memory,memory_read,memory_program,memory_erase}; }
static void run(unsigned ms) {
    uint64_t end=time_us+(uint64_t)ms*1000;
    while(time_us<end){runtime_poll();time_us+=100;}
}
static void send(const char *s) {
    assert(in_count==in_read);in_count=strlen(s);assert(in_count<sizeof input);memcpy(input,s,in_count);in_read=0;
}
static unsigned count(const char *s) {
    unsigned n=0;const char *p=output;while((p=strstr(p,s))){++n;p+=strlen(s);}return n;
}
int main(void) {
    flash_reset(&memory);runtime_init();run(300);send("version\n");run(10);assert(strstr(output,"VERSION source="));
    send("rxdiag\n");run(10);assert(strstr(output,"RXDIAG attempts="));assert(strstr(output,"RXHEALTH time_ms="));
    send("calibrate\n");run(10200);assert(strstr(output,"event=calibration_ready"));
    send("mode work\n");run(10);send("calibrate\n");run(10200);
    assert(count("event=calibration_ready")==2);
    unsigned before=probes;send("imuaddr 0x69\n");run(3100);
    assert(probes-before>=3&&probes-before<=4&&strstr(output,"event=imu_error"));
    pir_level=true;edge_pending=true;edge_high=true;edge_time=(uint32_t)time_us;run(10);assert(count("event=operator_motion")==1);
    send("imuaddr 0x68\n");run(1100);assert(strstr(output,"event=imu_recovered"));
    adc_failure=true;run(2500);assert(adc_failures==3);
    adc_failure=false;run(1100);assert(strstr(output,"event=adc_recovered"));
    send("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n");run(10);
    assert(strstr(output,"event=invalid_line"));
    unsigned status=count("event=status");rx_overflow=true;send("status\n");run(10);assert(count("event=status")==status);
    send("status\r\n");run(10);assert(count("event=status")==status+1);
    unsigned versions=count("VERSION source=");rx_overflow=true;send("bad\n");run(10);
    send("version\n");run(10);assert(count("VERSION source=")==versions+1);
    char flood[701]={0};for(unsigned i=0;i<100;++i)strcat(flood,"status\n");
    unsigned before_flood=count("event=status");send(flood);run(5);
    assert(count("event=status")-before_flood<=6);run(200);
    assert(count("event=status")-before_flood==100);
    memory.allowance=0;
    for(unsigned i=0;i<40;++i){send("mark\n");run(2);}
    run(1100);assert(strstr(output,"storage_fault=1"));
    assert(strstr(output,"queue_lost=24"));
    memory.allowance=-1;run(2100);
    /* Run >2000 periodic releases. Force 30 journal rotations and inject one PIR edge during each erase. */
    for(unsigned n=0;n<2050;++n){
        send("mark\n");run(22);
        if(pir_level){run(2);pir_level=false;edge_pending=true;edge_high=false;edge_time=(uint32_t)time_us;run(2);}
    }
    assert(erase_count>=30&&injected==erase_count&&count("event=operator_motion")==injected+1);
    send("stats\n");run(300);assert(strstr(output,"STAT name=imu")&&strstr(output,"MEM stack_used=0"));
    LwJournal reopened;assert(journal_open(&reopened,flash_io(&memory))&&reopened.damaged==0);
    printf("runtime HOST MODEL: %u erase injections, %u PIR records, %u durable records, parser overflow, NACK/recovery and ADC retry passed\n",injected,count("event=operator_motion"),reopened.valid);
    /* Keep raw output for audit when requested, never label it MCU evidence. */
    const char *save=getenv("LABWATCH_HOST_LOG");if(save){FILE *f=fopen(save,"w");assert(f);fwrite(output,1,out_count,f);fclose(f);}
    return 0;
}
