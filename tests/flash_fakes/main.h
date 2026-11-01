#ifndef LABWATCH_FLASH_TEST_MAIN_H
#define LABWATCH_FLASH_TEST_MAIN_H
#include <stdint.h>
#define HAL_OK 0
#define HAL_ERROR 1
#define FLASH_TYPEPROGRAM_DOUBLEWORD 0u
#define FLASH_TYPEERASE_PAGES 0u
typedef unsigned HAL_StatusTypeDef;
typedef struct { uint32_t TypeErase,Page,NbPages; } FLASH_EraseInitTypeDef;
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef HAL_FLASH_Lock(void);
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t,uintptr_t,uint64_t);
HAL_StatusTypeDef HAL_FLASHEx_Erase(FLASH_EraseInitTypeDef *,uint32_t *);
uintptr_t storage_test_address(void);
#endif
