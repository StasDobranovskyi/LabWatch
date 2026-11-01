#include "board.h"
#include "main.h"
#include "clock_ms.h"
static struct { uint32_t time; bool high; } edges[8];
static volatile unsigned edge_head,edge_tail;
static volatile uint32_t edge_lost;
void board_clock_init(void) {
    /* HAL_Init already starts SysTick. No second mutable clock state. */
}
uint32_t board_micros(void) {
    /* One atomic aligned HAL tick read; resolution is 1000 us, not 1 us. */
    return clock_ms_to_us(HAL_GetTick());
}
/* SysTick loss during a flash stall is not detectable with this clock. */
uint32_t board_clock_gaps(void) { return 0; }
void board_probe(unsigned task,bool on) {
    unsigned pin=task==0?6:task==1?7:10;
    GPIOA->BSRR=1u<<(pin+(on?0:16));
}
static void edge(bool high) {
    unsigned next=(edge_head+1)%8;
    if(next==edge_tail){++edge_lost;return;}
    edges[edge_head].time=board_micros();edges[edge_head].high=high;edge_head=next;
}
void HAL_GPIO_EXTI_Rising_Callback(uint16_t pin) { if(pin==GPIO_PIN_1)edge(true); }
void HAL_GPIO_EXTI_Falling_Callback(uint16_t pin) { if(pin==GPIO_PIN_1)edge(false); }
void EXTI0_1_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1); }
bool board_pir_event(bool *high,uint32_t *time) {
    if(edge_head==edge_tail)return false;
    *high=edges[edge_tail].high;*time=edges[edge_tail].time;edge_tail=(edge_tail+1)%8;return true;
}
uint32_t board_pir_lost(void) { return edge_lost; }
extern uint32_t _estack,_Min_Stack_Size;
#define PAINT 0xd15ea5edu
static uint32_t *bottom(void) { return (uint32_t *)((uintptr_t)&_estack-(uintptr_t)&_Min_Stack_Size); }
void board_stack_paint(void) {
    uint32_t *end=(uint32_t *)__get_MSP()-16;
    for(uint32_t *p=bottom();p<end;++p)*p=PAINT;
}
uint32_t board_stack_used(void) {
    uint32_t *p=bottom();while(p<&_estack&&*p==PAINT)++p;
    return (uint32_t)((uintptr_t)&_estack-(uintptr_t)p);
}
