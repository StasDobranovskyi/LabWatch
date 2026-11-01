#include "board.h"
#include "main.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint64_t backing[512];static bool no_op_erase,hal_failure,unlocked;
uintptr_t storage_test_address(void) { return (uintptr_t)backing; }
HAL_StatusTypeDef HAL_FLASH_Unlock(void) { unlocked=!hal_failure;return hal_failure?HAL_ERROR:HAL_OK; }
HAL_StatusTypeDef HAL_FLASH_Lock(void) { unlocked=false;return HAL_OK; }
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t type,uintptr_t at,uint64_t value) {
    (void)type;assert(unlocked&&at>=(uintptr_t)backing&&at+8<=(uintptr_t)backing+4096);
    memcpy((void *)at,&value,8);return HAL_OK;
}
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *r,uint32_t *error) {
    assert(unlocked&&r->Page>=14&&r->Page<=15&&r->NbPages==1);*error=UINT32_MAX;
    if(!no_op_erase)memset((uint8_t *)backing+(r->Page-14)*2048,255,2048);
    return HAL_OK;
}
int main(void) {
    LwStorage io=board_storage();uint8_t bytes[8]={1,2,3,4,5,6,7,8},actual[8];
    memset(backing,0,sizeof backing);no_op_erase=true;
    assert(!io.erase(NULL,0));LwFlashDiagnostic d=board_storage_diagnostic();
    assert(d.erases==1&&d.erase_rejected==1&&d.first_bad_offset==0&&d.first_bad_word==0);
    no_op_erase=false;assert(io.erase(NULL,0)&&!unlocked);
    for(unsigned i=0;i<2048;++i)assert(((uint8_t *)backing)[i]==255);
    assert(io.program(NULL,0,bytes)&&!unlocked&&io.read(NULL,0,actual,8)&&!memcmp(bytes,actual,8));
    no_op_erase=true;assert(!io.erase(NULL,1));d=board_storage_diagnostic();assert(d.first_bad_offset==2048);
    assert(!io.read(NULL,4090,actual,8)&&!io.program(NULL,4090,bytes)&&!io.erase(NULL,2));
    hal_failure=true;assert(!io.erase(NULL,1)&&!io.program(NULL,8,bytes));
    puts("flash adapter HOST: HAL_OK without erased bytes rejected, full-page read-back, lock discipline, bounds and HAL failure passed");return 0;
}
