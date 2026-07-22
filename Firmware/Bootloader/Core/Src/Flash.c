#include "Flash.h"
#include "stm32f0xx_hal.h"
#include "string.h"
#include "stdio.h"

#define FLASH_PAGE_SIZE_F0  2048 // 2 KB for STM32F070

uint8_t bytes_temp[4];

void float2Bytes(uint8_t * ftoa_bytes_temp,float float_variable)
{
    union {
      float a;
      uint8_t bytes[4];
    } thing;

    thing.a = float_variable;

    for (uint8_t i = 0; i < 4; i++) {
      ftoa_bytes_temp[i] = thing.bytes[i];
    }
}

float Bytes2float(uint8_t * ftoa_bytes_temp)
{
    union {
      float a;
      uint8_t bytes[4];
    } thing;

    for (uint8_t i = 0; i < 4; i++) {
    	thing.bytes[i] = ftoa_bytes_temp[i];
    }

   float float_variable =  thing.a;
   return float_variable;
}

void Convert_To_Str (uint32_t *Data, char *Buf)
{
	int numberofbytes = ((strlen((char *)Data)/4) + ((strlen((char *)Data) % 4) != 0)) *4;

	for (int i=0; i<numberofbytes; i++)
	{
		Buf[i] = Data[i/4]>>(8*(i%4));
	}
}

void Flash_Write_NUM (uint32_t StartSectorAddress, float Num)
{
	float2Bytes(bytes_temp, Num);
	WriteFlash (StartSectorAddress, (uint32_t *)bytes_temp, 1);
}

float Flash_Read_NUM (uint32_t StartSectorAddress)
{
	uint8_t buffer[4];
	float value;

	ReadFlash((uint32_t *)StartSectorAddress, (uint32_t *)buffer, 1);	value = Bytes2float(buffer);
	return value;
}

void ReadFlash (uint32_t *readAddr, uint32_t *readBuf, uint16_t wordCnt)
{
  memcpy(readBuf, readAddr , wordCnt * 4);
}

// --- New Bootloader Functions ---

/**
  * @brief  Erases the Main Application area page by page based on byte size.
  */
uint32_t Flash_EraseAppArea(uint32_t StartAddress, uint32_t SizeInBytes) {
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError;

    HAL_FLASH_Unlock();

    // Calculate the number of pages to erase (Round up)
    uint32_t numPages = (SizeInBytes + FLASH_PAGE_SIZE_F0 - 1) / FLASH_PAGE_SIZE_F0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = StartAddress;
    EraseInitStruct.NbPages = numPages;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        uint32_t error = HAL_FLASH_GetError();
        HAL_FLASH_Lock();
        return error;
    }

    HAL_FLASH_Lock();
    return 0;
}

/**
  * @brief  Writes Word (32-bit) directly to the specified address without erasing.
  */
uint32_t Flash_WriteWordsNoErase(uint32_t StartAddress, uint32_t *Data, uint32_t WordCount) {
    HAL_FLASH_Unlock();

    for (uint32_t i = 0; i < WordCount; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, StartAddress, Data[i]) == HAL_OK) {
            StartAddress += 4;
        } else {
            uint32_t error = HAL_FLASH_GetError();
            HAL_FLASH_Lock();
            return error;
        }
    }

    HAL_FLASH_Lock();
    return 0;
}

// Legacy functions
uint32_t EraseFlash(uint32_t StartAddress, uint16_t numberofwords) {
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError;
    HAL_FLASH_Unlock();
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = StartAddress;
    EraseInitStruct.NbPages = ((numberofwords * 4) / FLASH_PAGE_SIZE_F0) + 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        return HAL_FLASH_GetError();
    }
    return 0;
}

uint32_t WriteFlash(uint32_t StartAddress, uint32_t *Data, uint16_t numberofwords) {
    // Erase the required area first
    if(EraseFlash(StartAddress, numberofwords) != 0) return HAL_FLASH_GetError();

    HAL_FLASH_Unlock();
    for (uint32_t i = 0; i < numberofwords; i++) {
        // F0 supports WORD (32-bit) programming
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, StartAddress, Data[i]) == HAL_OK) {
            StartAddress += 4;
        } else {
            return HAL_FLASH_GetError();
        }
    }
    HAL_FLASH_Lock();
    return 0;
}
