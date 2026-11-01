#include "journal.h"
#include <string.h>
#define RECORD_MAGIC 0x3342574cu
#define RECORD_SEAL 0xc04d17edu
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
}
static void put32(uint8_t *p,uint32_t x) { for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(x>>(8*i)); }
uint32_t journal_crc(const uint8_t *p,size_t n) {
    uint32_t crc=0xffffffffu;
    while(n--){crc^=*p++;for(unsigned b=0;b<8;++b)crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);}
    return ~crc;
}
static bool erased(const uint8_t p[32]) {
    for(unsigned b=0;b<32;++b)if(p[b]!=255)return false;
    return true;
}
static bool decode(const uint8_t p[32],LwRecord *r) {
    if(get32(p)!=RECORD_MAGIC||get32(p+28)!=RECORD_SEAL||get32(p+24)!=journal_crc(p,24))return false;
    if(!get32(p+4)||p[12]<LW_MODE||p[12]>LW_CHECKPOINT||p[13]>LW_SERVICE||p[14]>LW_ERROR||p[15]!=1)return false;
    *r=(LwRecord){get32(p+4),get32(p+8),(LwEvent)p[12],(LwMode)p[13],(LwLevel)p[14],
                  (int32_t)get32(p+16),(int32_t)get32(p+20)};
    return true;
}
bool journal_record(LwJournal *j,unsigned i,LwRecord *r) {
    uint8_t p[32];return i<LW_JOURNAL_SLOTS&&j->io.read(j->io.context,i*32,p,32)&&decode(p,r);
}
bool journal_open(LwJournal *j,LwStorage io) {
    memset(j,0,sizeof *j);j->io=io;
    if(!io.read||!io.program||!io.erase)return false;
    for(unsigned i=0;i<LW_JOURNAL_SLOTS;++i) {
        uint8_t p[32];LwRecord r;
        if(!io.read(io.context,i*32,p,32))return false;
        if(decode(p,&r)){
            ++j->valid;
            if(!j->present||r.sequence>j->last_sequence){j->present=true;j->last_sequence=r.sequence;j->newest=i;}
        } else if(!erased(p))++j->damaged;
    }
    return true;
}
bool journal_append(LwJournal *j,const LwRecord *r) {
    if(!r->sequence||(j->present&&r->sequence<=j->last_sequence)||r->event<LW_MODE||r->event>LW_CHECKPOINT||
       r->mode>LW_SERVICE||r->level>LW_ERROR)return false;
    unsigned target=LW_JOURNAL_SLOTS;
    for(unsigned step=0;step<LW_JOURNAL_SLOTS;++step) {
        unsigned i=(j->present?j->newest+1+step:step)%LW_JOURNAL_SLOTS;uint8_t p[32];
        if(!j->io.read(j->io.context,i*32,p,32))return false;
        if(erased(p)){target=i;break;}
    }
    if(target==LW_JOURNAL_SLOTS) {
        /* Never erase the page holding the latest durable event. */
        unsigned victim=j->present?1-j->newest/64:0;
        if(!j->io.erase(j->io.context,victim))return false;
        target=victim*64;
        if(!journal_open(j,j->io))return false;
    }
    uint8_t p[32]={0};
    put32(p,RECORD_MAGIC);put32(p+4,r->sequence);put32(p+8,r->time_ms);
    p[12]=(uint8_t)r->event;p[13]=(uint8_t)r->mode;p[14]=(uint8_t)r->level;p[15]=1;
    put32(p+16,(uint32_t)r->value);put32(p+20,(uint32_t)r->detail);
    put32(p+24,journal_crc(p,24));put32(p+28,RECORD_SEAL);
    /* Commit CRC+seal last. A torn slot is never programmed again. */
    for(unsigned part=0;part<4;++part) {
        if(!j->io.program(j->io.context,target*32+part*8,p+part*8))break;
    }
    LwRecord verified;
    if(!journal_record(j,target,&verified)||verified.sequence!=r->sequence)return false;
    j->last_sequence=r->sequence;j->newest=target;j->present=true;++j->valid;
    return true;
}
