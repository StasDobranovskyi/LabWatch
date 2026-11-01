#include "flash_fake.h"
#include "perf.h"
#include <stdio.h>
int main(void) {
    FlashFake f;LwJournal j;
    assert(journal_crc((const uint8_t *)"123456789",9)==0xcbf43926u);
    unsigned cuts=0;
    for(unsigned half=0;half<4;++half)for(unsigned bytes=0;bytes<8;++bytes) {
        flash_reset(&f);assert(journal_open(&j,flash_io(&f)));
        LwRecord r=record(1);assert(journal_append(&j,&r));
        f.allowance=(int)half;f.partial=bytes;r=record(2);
        assert(!journal_append(&j,&r));
        assert(journal_open(&j,flash_io(&f))&&j.last_sequence==1&&j.valid==1);
        f.allowance=-1;r=record(3);assert(journal_append(&j,&r));
        assert(journal_open(&j,flash_io(&f))&&j.last_sequence==3&&j.valid==2);++cuts;
    }
    flash_reset(&f);assert(journal_open(&j,flash_io(&f)));
    for(unsigned n=1;n<=300;++n){LwRecord r=record(n);assert(journal_append(&j,&r));}
    assert(f.erases==3&&j.last_sequence==300&&j.valid==108);
    assert(journal_open(&j,flash_io(&f))&&j.last_sequence==300&&j.valid==108);
    f.bytes[j.newest*32+16]^=1;assert(journal_open(&j,flash_io(&f))&&j.last_sequence==299&&j.damaged==1);
    flash_reset(&f);assert(journal_open(&j,flash_io(&f)));
    for(unsigned n=1;n<=128;++n){LwRecord r=record(n);assert(journal_append(&j,&r));}
    f.erase_fail=true;LwRecord r=record(129);assert(!journal_append(&j,&r));
    assert(journal_open(&j,flash_io(&f))&&j.last_sequence==128);
    f.erase_fail=false;assert(journal_append(&j,&r));
    r=record(129);assert(!journal_append(&j,&r));
    flash_reset(&f);assert(journal_open(&j,flash_io(&f)));
    f.return_after_commit=true;r=record(1);assert(journal_append(&j,&r));
    assert(journal_open(&j,flash_io(&f))&&j.valid==1&&j.last_sequence==1);
    f.read_fail=true;assert(!journal_open(&j,flash_io(&f)));
    uint32_t release=UINT32_MAX-499,skipped;
    assert(!perf_due(release-1,&release,1000,&skipped));
    assert(perf_due(release,&release,1000,&skipped)&&skipped==0);
    assert(release==500);assert(perf_due(3501,&release,1000,&skipped)&&skipped==3&&release==4500);
    LwStat s={0};perf_release(&s,UINT32_MAX-1000,20000,0);perf_add(&s,100);
    perf_release(&s,19049,20000,2);perf_add(&s,150);
    assert(s.jitter_max==50&&s.missed==2&&s.min_us==100&&s.max_us==150);
    printf("journal host model: %u torn-write cuts; 300 appends/3 rotations, corrupted CRC, interrupted erase, sequence monotonicity and us rollover passed\n",cuts);
    return 0;
}
