#ifndef LABWATCH_FLASH_FAKE_H
#define LABWATCH_FLASH_FAKE_H
#include "journal.h"
#include <assert.h>
#include <string.h>
typedef struct { uint8_t bytes[4096];int allowance;unsigned partial;unsigned erases;bool read_fail,erase_fail,return_after_commit; } FlashFake;
static inline void flash_reset(FlashFake *f) { memset(f,0,sizeof *f);memset(f->bytes,255,sizeof f->bytes);f->allowance=-1; }
static inline bool flash_read(void *ctx,unsigned at,void *to,size_t n) {
    FlashFake *f=ctx;assert(at+n<=4096);if(f->read_fail)return false;memcpy(to,f->bytes+at,n);return true;
}
static inline bool flash_write(void *ctx,unsigned at,const uint8_t p[8]) {
    FlashFake *f=ctx;assert(at%8==0&&at+8<=4096);
    unsigned n=f->allowance==0?f->partial:8;
    for(unsigned i=0;i<n;++i){assert((f->bytes[at+i]&p[i])==p[i]);f->bytes[at+i]&=p[i];}
    if(f->allowance==0)return false;
    if(f->allowance>0)--f->allowance;
    return !(f->return_after_commit&&at%32==24);
}
static inline bool flash_erase(void *ctx,unsigned page) {
    FlashFake *f=ctx;assert(page<2);++f->erases;
    if(f->erase_fail){for(unsigned i=0;i<1024;++i)f->bytes[page*2048+i]=255;return false;}
    memset(f->bytes+page*2048,255,2048);return true;
}
static inline LwStorage flash_io(FlashFake *f) { return (LwStorage){f,flash_read,flash_write,flash_erase}; }
static inline LwRecord record(uint32_t n) { return (LwRecord){n,n*20,LW_SHOCK,LW_WORK,LW_ALARM,1200,1100}; }
#endif
