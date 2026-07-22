/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "Flash.h"
#include "usbd_cdc_if.h"
#include "st7789.h"

// Flash Addresses
#define BOOTLOADER_FLASH_ADDR       0x08000000
#define PARAM_PAGE_ADDR			    0x0801F800    // Starts after 32KB Bootloader
#define MAIN_APP_FLASH_ADDR         0x08008000    // Starts after 2KB Config Page
#define VECTOR_TABLE_SIZE  0xC0U





// Protocol Commands
#define CMD_START                  0x01
#define CMD_DATA                   0x02
#define CMD_END                    0x03
#define CMD_JUMP_ONLY              0x04
#define CMD_ERASE_CRC              0x05
#define CMD_JUMP_ONLY              0x04
#define CMD_ERASE_CRC              0x05
// Protocol Responses
#define ACK                        0x06
#define NACK                       0x15


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_CRC_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char MBCodeBuiltDateTime[30];
uint8_t flashbuffer[128] __attribute__((aligned(4)));

uint8_t usb_rx_buffer[256];
volatile uint32_t usb_rx_len = 0;
volatile uint8_t usb_data_ready = 0;

uint32_t write_address = MAIN_APP_FLASH_ADDR;
uint32_t total_file_size = 0;
uint8_t bootloader_active = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
void PrintBuildDate(void);
void WriteToDevice(char* commandString);
void JumpToMainApp(void);
uint32_t calculate_crc32(uint8_t *data, uint32_t size);
void ReadFlashBuffer(void);
void WriteCRC(unsigned char *data);
uint32_t ReadCRC(void);
void WriteAppSize(uint32_t size);
uint32_t ReadAppSize(void);
void Bootloader_Protocol_Handler(void);
void Send_USB_Response(uint8_t res);

void PrintBuildDate(void)
{
    snprintf(MBCodeBuiltDateTime, 30, "%s %s", __DATE__, __TIME__);
    WriteToDevice("Code Build Date: ");
    WriteToDevice(MBCodeBuiltDateTime);
    WriteToDevice("\r\n\r\n");
}

void WriteToDevice(char* commandString)
{
    CDC_Transmit_FS((uint8_t*)commandString, strlen(commandString));
}


void JumpToMainApp(void)
{
    uint32_t appStack = *(volatile uint32_t*)MAIN_APP_FLASH_ADDR;
    uint32_t appEntry = *(volatile uint32_t*)(MAIN_APP_FLASH_ADDR + 4);

    if ((appStack < 0x20000000U) || (appStack > 0x20004000U))
        return;

    __disable_irq();

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    USB->CNTR = USB_CNTR_FRES;
    USB->CNTR = 0;
    USB->BCDR &= ~USB_BCDR_DPPU;
    __HAL_RCC_USB_CLK_DISABLE();

    for (uint32_t i = 0; i < 8; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    for (uint32_t i = 0; i < VECTOR_TABLE_SIZE / 4; i++)
    {
        ((volatile uint32_t*)0x20000000U)[i] =
            ((volatile uint32_t*)MAIN_APP_FLASH_ADDR)[i];
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();

    SYSCFG->CFGR1 &= ~SYSCFG_CFGR1_MEM_MODE;
    SYSCFG->CFGR1 |= SYSCFG_CFGR1_MEM_MODE_0 | SYSCFG_CFGR1_MEM_MODE_1;

    __DSB();
    __ISB();

    __set_MSP(appStack);

    ((void (*)(void))appEntry)();

    while (1);
}


uint32_t calculate_crc32(uint8_t *data, uint32_t size)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < size; i++)
    {
        crc = crc ^ (uint32_t)data[i];
        for (uint32_t j = 0; j < 8; j++)
        {
            if (crc & 0x80000000) crc = (crc << 1) ^ 0x04C11DB7;
            else crc <<= 1;
        }
    }
    return crc;
}

void ReadFlashBuffer(void)
{
    ReadFlash((uint32_t*)PARAM_PAGE_ADDR, (uint32_t*)flashbuffer, 32);
}

void WriteCRC(unsigned char *data)
{
    ReadFlashBuffer();
    flashbuffer[16] = data[0]; flashbuffer[17] = data[1];
    flashbuffer[18] = data[2]; flashbuffer[19] = data[3];
    WriteFlash(PARAM_PAGE_ADDR, (uint32_t*)flashbuffer, sizeof(flashbuffer)/sizeof(uint32_t));
    HAL_Delay(10);
}

void WriteBootParams(uint32_t crc, uint32_t size)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;

    HAL_FLASH_Unlock();

    // Sadece parametre sayfasını sil (Uygulamaya asla dokunmaz)
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = PARAM_PAGE_ADDR;
    EraseInitStruct.NbPages = 1;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    // CRC ve Boyut bilgilerini mühürle
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, PARAM_PAGE_ADDR, crc);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, PARAM_PAGE_ADDR + 4, size);

    HAL_FLASH_Lock();
}


uint32_t ReadCRC(void)
{
    return *(volatile uint32_t*)PARAM_PAGE_ADDR;
}

uint32_t ReadAppSize(void)
{
    return *(volatile uint32_t*)(PARAM_PAGE_ADDR + 4);
}

void WriteAppSize(uint32_t size)
{
    ReadFlashBuffer();
    flashbuffer[20] = (uint8_t)(size & 0xFF);
    flashbuffer[21] = (uint8_t)((size >> 8) & 0xFF);
    flashbuffer[22] = (uint8_t)((size >> 16) & 0xFF);
    flashbuffer[23] = (uint8_t)((size >> 24) & 0xFF);
    WriteFlash(PARAM_PAGE_ADDR, (uint32_t*)flashbuffer, sizeof(flashbuffer)/sizeof(uint32_t));
    HAL_Delay(10);
}



void Send_USB_Response(uint8_t res)
{
    uint32_t start_tick = HAL_GetTick();
    while (CDC_Transmit_FS(&res, 1) == USBD_BUSY)
    {
        if ((HAL_GetTick() - start_tick) > 100) break;
        HAL_Delay(1);
    }
}

void Bootloader_Protocol_Handler(void)
{
    if (!usb_data_ready) return;
    uint8_t cmd = usb_rx_buffer[0];

    if (cmd == CMD_START)
        {
            total_file_size = usb_rx_buffer[1] | (usb_rx_buffer[2] << 8) | (usb_rx_buffer[3] << 16) | (usb_rx_buffer[4] << 24);
            write_address = MAIN_APP_FLASH_ADDR;
            bootloader_active = 1;

            //Bootloader_ShowProgress(0, total_file_size);
            WriteBootParams(0xFFFFFFFF, 0xFFFFFFFF);  // Delete existing CRC

            if (Flash_EraseAppArea(MAIN_APP_FLASH_ADDR, total_file_size) == 0) Send_USB_Response(ACK);
            else Send_USB_Response(NACK);
        }

    else if (cmd == CMD_DATA && bootloader_active)
        {
            uint16_t len = usb_rx_buffer[1] | (usb_rx_buffer[2] << 8);
            uint8_t *payload = &usb_rx_buffer[3];

            // DİKKAT: Veriyi 4'ün katına (Word Alignment) yuvarlıyoruz!
            uint32_t word_count = (len + 3) / 4;
            uint32_t aligned_buffer[64]; // Maksimum paket boyutunu kaldıracak havuz

            // 1. Buffer'ı 0xFF ile doldur (Boş kalan byte'lar Flash'ta 0xFF olarak kalmalıdır)
            memset(aligned_buffer, 0xFF, sizeof(aligned_buffer));

            // 2. Gelen 8-bitlik raw veriyi güvenli 32-bitlik havuza kopyala
            memcpy(aligned_buffer, payload, len);

            // 3. Özel Flash.c fonksiyonumuzu çağırarak veriyi donanıma kazı
            if (Flash_WriteWordsNoErase(write_address, aligned_buffer, word_count) == 0)
            {
                write_address += (word_count * 4); // Adresi başarılı yazılan byte kadar ileri sar
                uint32_t writtenBytes = write_address - MAIN_APP_FLASH_ADDR;
                //Bootloader_ShowProgress(writtenBytes, total_file_size);
                Send_USB_Response(ACK);
            }
            else
            {
                Send_USB_Response(NACK); // Yazma hatası oluştuysa Python'a bildir
            }
        }
    else if (cmd == CMD_END && bootloader_active)
        {
            uint32_t received_crc = usb_rx_buffer[1] | (usb_rx_buffer[2] << 8) | (usb_rx_buffer[3] << 16) | (usb_rx_buffer[4] << 24);
            uint32_t calculated_flash_crc = calculate_crc32((uint8_t*)MAIN_APP_FLASH_ADDR, total_file_size);

            if (calculated_flash_crc == received_crc)
            {
                // Python'dan gelen ile hesaplanan eşleşti! Güvenle parametreyi mühürle ve zıpla!
                WriteBootParams(calculated_flash_crc, total_file_size);
                Send_USB_Response(ACK);
                HAL_Delay(200);
                JumpToMainApp();
            }
            else
            {
            uint8_t error_packet[5] = {NACK};
            error_packet[1] = (uint8_t)(calculated_flash_crc & 0xFF);
            error_packet[2] = (uint8_t)((calculated_flash_crc >> 8) & 0xFF);
            error_packet[3] = (uint8_t)((calculated_flash_crc >> 16) & 0xFF);
            error_packet[4] = (uint8_t)((calculated_flash_crc >> 24) & 0xFF);
            CDC_Transmit_FS(error_packet, 5);
        }
        bootloader_active = 0;
    }
    usb_data_ready = 0;
}

void Bootloader_ShowProgress(uint32_t writtenBytes, uint32_t totalBytes)
{
    char text[20];

    if (totalBytes == 0)
        return;

    uint32_t percent = (writtenBytes * 100U) / totalBytes;

    if (percent > 100)
        percent = 100;

    snprintf(text, sizeof(text), "%lu%%", percent);

    // Ortaya yakın bölgeyi temizle
    ST7789_DrawFilledRectangle(70, 90, 180, 140, BLACK);

    // Başlık
    ST7789_WriteString(55, 70, "Uploading", Font_11x18, WHITE, BLACK);

    // Yüzde yazısı
    ST7789_WriteString(105, 100, text, Font_16x26, GREEN, BLACK);

    // Basit progress bar çerçevesi
    ST7789_DrawRectangle(40, 135, 280, 155, WHITE);

    // İç dolum
    uint16_t barWidth = (uint16_t)((236U * percent) / 100U);
    ST7789_DrawFilledRectangle(42, 137, 42 + barWidth, 153, GREEN);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USB_DEVICE_Init();
  MX_CRC_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  __HAL_RCC_USB_CLK_ENABLE();
  USB->BCDR |= USB_BCDR_DPPU;


  HAL_Delay(100);
  PrintBuildDate();

  ST7789_Init();

  ST7789_WriteString(0, 0, "BOOTLOADER v1.0", Font_11x18, WHITE, BLUE);
  HAL_Delay(100);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */


	    // ====================================================================
	    // 1. ADIM: OTOMATİK ATLAMA GÜVENLİK KONTROLÜ (GÜÇ AÇILIŞI)
	    // ====================================================================
	    uint32_t expected_crc = ReadCRC();
	    uint32_t saved_app_size = ReadAppSize();

	    // Hafızada geçerli bir CRC ve mantıklı bir uygulama boyutu var mı? (Örn: max 96KB)
	    if (expected_crc != 0xFFFFFFFF && saved_app_size > 0 && saved_app_size <= (96 * 1024))
	    {
	        // Flash'taki uygulamanın gerçek CRC'sini hesapla
	        uint32_t current_flash_crc = calculate_crc32((uint8_t*)MAIN_APP_FLASH_ADDR, saved_app_size);

	        if (current_flash_crc == expected_crc)
	        {
	            // EĞER HER ŞEY DOĞRUYSA: Hiç bekleme yapmadan DOĞRUDAN ana uygulamaya zıpla!
	            JumpToMainApp();
	        }
	    }

	    // ====================================================================
	    // 2. ADIM: EĞER GEÇERLİ UYGULAMA YOKSA VEYA UPLOADER BAĞLIYSA
	    // ====================================================================
	    WriteToDevice("No Valid App or CRC Mismatch. Staying in Bootloader Mode...\r\n");

	    // Ana Döngü: Gelen tüm uploader komutlarını ve ham test butonlarını dinler
	    while (1)
	    {
	        // Bootloader modunda çakılı kaldığını göstermek için LED'leri flaşör yap
	        static uint32_t last_wait_hb = 0;
	        if ((HAL_GetTick() - last_wait_hb) > 150)
	        {
	            HAL_GPIO_TogglePin(GPIOB, LED_BLUE_Pin);
	            HAL_GPIO_TogglePin(GPIOB, LED_RED_Pin);
	            last_wait_hb = HAL_GetTick();
	        }

	        if (usb_data_ready)
	        {
	            uint8_t cmd = usb_rx_buffer[0];

	            if (cmd == CMD_JUMP_ONLY)
	            {
	                WriteToDevice("Ham Emir: Uygulamaya atlanıyor...\r\n");
	                Send_USB_Response(ACK);
	                HAL_Delay(100);
	                JumpToMainApp();
	            }
	            else if (cmd == CMD_ERASE_CRC)
	            {
	                WriteToDevice("Ham Emir: CRC Alanı temizleniyor...\r\n");
	                uint8_t invalid_crc[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	                WriteCRC(invalid_crc);
	                Send_USB_Response(ACK);
	            }
	            else
	            {
	                // Protokolün can damarı: CMD_START, CMD_DATA ve CMD_END akışını işletir
	                // Dosya yüklemesi bittiğinde kendisi otomatik olarak WriteCRC ve JumpToMainApp çağıracak!
	                Bootloader_Protocol_Handler();
	            }

	            usb_data_ready = 0; // Bayrağı indir
	        }
	    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_1LINE;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, TFT_DC_Pin|TFT_RES_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LED_RED_Pin|LED_BLUE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : TFT_DC_Pin TFT_RES_Pin */
  GPIO_InitStruct.Pin = TFT_DC_Pin|TFT_RES_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED_RED_Pin LED_BLUE_Pin */
  GPIO_InitStruct.Pin = LED_RED_Pin|LED_BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
      HAL_GPIO_TogglePin(GPIOB, LED_RED_Pin);
      HAL_Delay(50);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
