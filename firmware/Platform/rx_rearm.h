#ifndef LABWATCH_RX_REARM_H
#define LABWATCH_RX_REARM_H
#include <stdbool.h>
#include <stdint.h>
typedef struct { uint32_t attempts, failures; int last_status; bool pending; } LwRxRearm;
typedef int (*LwRxArm)(void *context);
void rx_rearm_request(LwRxRearm *state);
bool rx_rearm_service(LwRxRearm *state, LwRxArm arm, void *context);
#endif
