#ifndef LABWATCH_JOURNAL_H
#define LABWATCH_JOURNAL_H
#include "instrument.h"
#include <stddef.h>
#define LW_PAGE_BYTES 2048u
#define LW_RECORD_BYTES 32u
#define LW_JOURNAL_SLOTS 128u
/* Offsets are relative to the reserved 4 KiB region. All writes are 8-byte aligned. */
typedef struct {
    void *context;
    bool (*read)(void *,unsigned,void *,size_t);
    bool (*program)(void *,unsigned,const uint8_t[8]);
    bool (*erase)(void *,unsigned);
} LwStorage;
typedef struct {
    LwStorage io; uint32_t last_sequence, valid, damaged;
    unsigned newest; bool present;
} LwJournal;
uint32_t journal_crc(const uint8_t *,size_t);
bool journal_open(LwJournal *,LwStorage);
bool journal_append(LwJournal *,const LwRecord *);
bool journal_record(LwJournal *,unsigned,LwRecord *);
#endif
