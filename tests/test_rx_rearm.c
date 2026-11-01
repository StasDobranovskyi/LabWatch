#include "rx_rearm.h"
#include <assert.h>
#include <stdio.h>
typedef struct { int statuses[4]; unsigned used; } Fake;
static int arm(void *context) {
    Fake *fake=context;
    return fake->statuses[fake->used++];
}
int main(void) {
    LwRxRearm state={0};Fake fake={{2,1,0,0},0};
    assert(rx_rearm_service(&state,arm,&fake));assert(fake.used==0);
    rx_rearm_request(&state);
    assert(!rx_rearm_service(&state,arm,&fake)); /* HAL_BUSY */
    assert(state.pending && state.attempts==1 && state.failures==1 && state.last_status==2);
    assert(!rx_rearm_service(&state,arm,&fake)); /* HAL_ERROR */
    assert(state.pending && state.attempts==2 && state.failures==2 && state.last_status==1);
    assert(rx_rearm_service(&state,arm,&fake)); /* foreground retry succeeds */
    assert(!state.pending && state.attempts==3 && state.failures==2 && state.last_status==0);
    rx_rearm_request(&state);assert(rx_rearm_service(&state,arm,&fake));
    assert(!state.pending && state.attempts==4 && state.failures==2);
    puts("rx rearm: busy/error retained, foreground retry and next receive passed");
    return 0;
}
