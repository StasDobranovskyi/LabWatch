#include "board.h"
#include "main.h"
#include <string.h>
#ifdef LW_STORAGE_HOST_TEST
#define JOURNAL_BASE storage_test_address()
#else
#define JOURNAL_BASE 0x08007000u
#endif
static LwFlashDiagnostic diagnostic;
static bool storage_read(void *ctx,unsigned offset,void *p,size_t n) {
    (void)ctx;if(offset>4096||n>4096-offset)return false;
    memcpy(p,(const void *)(JOURNAL_BASE+offset),n);return true;
}
static bool storage_program(void *ctx,unsigned offset,const uint8_t p[8]) {
    (void)ctx;if(offset>4088||offset%8)return false;
    uint64_t value;memcpy(&value,p,8);
    if(HAL_FLASH_Unlock()!=HAL_OK)return false;
    HAL_StatusTypeDef status=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,JOURNAL_BASE+offset,value);
    HAL_FLASH_Lock();return status==HAL_OK;
}
static bool storage_erase(void *ctx,unsigned page) {
    (void)ctx;if(page>1)return false;
    ++diagnostic.erases;
    FLASH_EraseInitTypeDef erase={0};uint32_t bad_page;
    erase.TypeErase=FLASH_TYPEERASE_PAGES;erase.Page=14+page;erase.NbPages=1;
    if(HAL_FLASH_Unlock()!=HAL_OK)return false;
    HAL_StatusTypeDef status=HAL_FLASHEx_Erase(&erase,&bad_page);
    HAL_FLASH_Lock();
    if(status!=HAL_OK)return false;
    const volatile uint32_t *words=(const volatile uint32_t *)(JOURNAL_BASE+page*2048u);
    for(unsigned i=0;i<512;++i)if(words[i]!=UINT32_MAX) {
        ++diagnostic.erase_rejected;diagnostic.first_bad_offset=page*2048u+i*4u;
        diagnostic.first_bad_word=words[i];return false;
    }
    return true;
}
LwFlashDiagnostic board_storage_diagnostic(void) { return diagnostic; }
LwStorage board_storage(void) { return (LwStorage){NULL,storage_read,storage_program,storage_erase}; }
