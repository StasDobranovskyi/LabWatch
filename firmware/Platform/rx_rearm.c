#include "rx_rearm.h"
void rx_rearm_request(LwRxRearm *state) { state->pending = true; }
bool rx_rearm_service(LwRxRearm *state, LwRxArm arm, void *context) {
    if (!state->pending) return true;
    ++state->attempts;
    state->last_status = arm(context);
    if (state->last_status != 0) { ++state->failures; return false; }
    state->pending = false;
    return true;
}
