/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdbool.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY_Pin GPIO_PIN_12
#define KEY_GPIO_Port GPIOB
#define S2_Pin GPIO_PIN_13
#define S2_GPIO_Port GPIOB
#define EN1_Pin GPIO_PIN_15
#define EN1_GPIO_Port GPIOB
#define SCL3_Pin GPIO_PIN_6
#define SCL3_GPIO_Port GPIOC
#define SDA3_Pin GPIO_PIN_7
#define SDA3_GPIO_Port GPIOC
#define SCL2_Pin GPIO_PIN_8
#define SCL2_GPIO_Port GPIOC
#define SDA2_Pin GPIO_PIN_9
#define SDA2_GPIO_Port GPIOC
#define SCL1_Pin GPIO_PIN_8
#define SCL1_GPIO_Port GPIOA
#define SDA1_Pin GPIO_PIN_9
#define SDA1_GPIO_Port GPIOA
#define ALERT__Pin GPIO_PIN_10
#define ALERT__GPIO_Port GPIOA
#define EN2_Pin GPIO_PIN_15
#define EN2_GPIO_Port GPIOA
#define TFT_DC_Pin GPIO_PIN_10
#define TFT_DC_GPIO_Port GPIOC
#define TFT_RES_Pin GPIO_PIN_11
#define TFT_RES_GPIO_Port GPIOC
#define EN3_Pin GPIO_PIN_12
#define EN3_GPIO_Port GPIOC
#define SPI1_CS_Pin GPIO_PIN_2
#define SPI1_CS_GPIO_Port GPIOD
#define EN4_Pin GPIO_PIN_6
#define EN4_GPIO_Port GPIOB
#define EN5_Pin GPIO_PIN_7
#define EN5_GPIO_Port GPIOB
#define LED_RED_Pin GPIO_PIN_8
#define LED_RED_GPIO_Port GPIOB
#define LED_BLUE_Pin GPIO_PIN_9
#define LED_BLUE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
extern volatile uint32_t UptimeMillis;
uint8_t read_register(uint8_t reg_addr);
extern volatile bool rotaryPressed;
void OnRotaryUP(bool Fast);
void OnRotaryDown(bool Fast);
void USB_SendMessage(const char *message);
void OnRotaryPressed(void);
//void PrepData(ADC_ChannelData channels[]) ;
void DrawChannelCircle(uint8_t row, bool enabled);
void DrawTableWithGrid(bool isMainMenu);
void DrawMenuArrowsAndCirclesInit(void);
void RedrawAllTableRows(void);
void PrintMenuSelection(void);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
