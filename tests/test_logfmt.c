#include "logfmt.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static char last[512];static unsigned calls;static bool full;
bool board_write_line(const char *p,size_t n) {
    ++calls;if(full)return false;assert(n<sizeof last);memcpy(last,p,n);last[n]=0;return true;
}
int main(void) {
    assert(lw_printf("%s %ld %lu %u %d\r\n","LabWatch",(long)INT32_MIN,(unsigned long)UINT32_MAX,7u,-9)>0);
    assert(!strcmp(last,"LabWatch -2147483648 4294967295 7 -9\r\n"));
    char large[600];memset(large,'x',sizeof large-1);large[599]=0;
    unsigned before=calls;assert(lw_printf("%s",large)==-1&&calls==before);
    full=true;assert(lw_printf("full\n")==-1&&calls==before+1);
    puts("log formatter: signed limits, unsigned limits, whole-line buffer rejection and TX backpressure passed");return 0;
}
