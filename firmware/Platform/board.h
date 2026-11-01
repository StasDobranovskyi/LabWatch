#ifndef PROJECT_BOARD_H
#define PROJECT_BOARD_H
#include "mpu6050.h"
#include "journal.h"
#include <stdbool.h>
#include <stdint.h>
void board_init(void);
SensorBus board_sensor_bus(void);
bool board_read_light(uint16_t *value);
bool board_motion(void);
void board_led(bool on);
int board_get_char(void);
int board_get_char_at(uint32_t *);
uint32_t board_micros(void);
void board_clock_init(void);
uint32_t board_clock_gaps(void);
void board_probe(unsigned,bool);
bool board_pir_event(bool *,uint32_t *);
uint32_t board_pir_lost(void);
LwStorage board_storage(void);
typedef struct { uint32_t erases,erase_rejected,first_bad_offset,first_bad_word; } LwFlashDiagnostic;
LwFlashDiagnostic board_storage_diagnostic(void);
void board_stack_paint(void);
uint32_t board_stack_used(void);
uint32_t board_heap_peak(void);
bool board_rx_overflow(void);
void board_rx_service(void);
typedef struct { uint32_t attempts, failures, hal_state, hal_error; int last_status; bool pending, irq_enabled, overrun; } LwRxDiagnostic;
LwRxDiagnostic board_rx_diagnostic(void);
int board_put_char(int value);
bool board_write_line(const char *,size_t);
bool board_tx_overflow(void);
#endif
