#ifndef INC_FLASH_SECTOR_F0_H_
#define INC_FLASH_SECTOR_F0_H_

#include "stdint.h"

// Existing functions
uint32_t WriteFlash (uint32_t StartSectorAddress, uint32_t *Data, uint16_t numberofwords);
void ReadFlash (uint32_t *readAddr, uint32_t *readBuf, uint16_t wordCnt);
void Convert_To_Str (uint32_t *Data, char *Buf);
void Flash_Write_NUM (uint32_t StartSectorAddress, float Num);
float Flash_Read_NUM (uint32_t StartSectorAddress);

// --- New Secure Functions for Bootloader ---
uint32_t Flash_EraseAppArea(uint32_t StartAddress, uint32_t SizeInBytes);
uint32_t Flash_WriteWordsNoErase(uint32_t StartAddress, uint32_t *Data, uint32_t WordCount);

#endif /* INC_FLASH_SECTOR_F0_H_ */
