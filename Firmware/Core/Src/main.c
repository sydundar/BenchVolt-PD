/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7789.h"
#include "usbd_cdc_if.h"
#include <stdio.h>
#include "stm32_sw_i2c.h"
#include "stusb4500.h"
#include "USBPD_CUST_NVM_API.h"
#include "USB_PD_defines.h"
#include <soft_i2c_2.h>
#include <soft_i2c_3.h>
#include "stm32f0xx_hal_cortex.h"
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
ADC_HandleTypeDef hadc;

SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void USB_SendMessage(const char *message) {
    // Check if the message is not NULL
    if (message != NULL) {
        CDC_Transmit_FS((uint8_t *)message, strlen(message));
    }
}

const char *welcomeMessage =
"-------------------------------------\r\n"
" Welcome to Fusion USB PD Power Supply\r\n"
" Distributing Power with USB PD Technology\r\n"
"-------------------------------------\r\n"
" System Ready\r\n"
" Firmware Version: 1.0.0\r\n"
" Developed by: Suleyman Y. Dundar\r\n"
"-------------------------------------\r\n";

typedef struct {
    char voltage[6]; // Voltaj değeri
    char current[6]; // Akım değeri
    char power[6];   // Güç değeri
} TableRow;

TableRow tableData[6] = {
    {"00.00", "00.00", "00.00"}, // 1.8V A W
    {"00.00", "00.00", "00.00"}, // 2.5V A W
    {"00.00", "00.00", "00.00"}, // 3.3V A W
    {"00.00", "00.00", "00.00"}, // Vlow A W
    {"00.00", "00.00", "00.00"}, // Vhigh A W
    {"00.00", "00.00", "00.00"}  // Vsink A V (VUSB)
};


float Curr3V3=0;
float Curr2V5=0;
float Curr1V8=0;
float CurrVLow=0;
float CurrVHigh=0;
float CurrVSink=0;

float Currlimit3V3 = 2.00f;
float Currlimit2V5 = 2.00f;
float Currlimit1V8 = 2.00f;
float CurrlimitVLow = 2.00f;
float CurrlimitVHigh = 1.00f;
float CurrlimitVSink = 5.00f;

float Volt3V3 = 3.30f;
float Volt2V5 = 2.50f;
float Volt1V8 = 1.80f;
float VoltVLow = 5.00f;
float VoltVHigh = 12.00f;
float VoltVSink = 20.00f;

// Max allowed limits
const float MaxCurrlimit3V3    = 2.00f;
const float MaxCurrlimit2V5    = 2.00f;
const float MaxCurrlimit1V8    = 2.00f;
const float MaxCurrlimitVLow   = 2.00f;
const float MaxCurrlimitVHigh  = 2.00f;
const float MaxCurrlimitVSink  = 2.00f;

//const float MaxVolt3V3         = 3.30f;
//const float MaxVolt2V5         = 2.50f;
//const float MaxVolt1V8         = 1.80f;
const float MinVoltVLow        = 0.50f;
const float MinVoltVHigh       = 2.50f;
const float MaxVoltVLow        = 5.00f;
const float MaxVoltVHigh       = 32.00f;
const float MaxVoltVSink       = 20.00f;

TableRow tableDataMenu[6] = {
    {"1.80", "2.00"}, // 1.8V A
    {"2.50", "2.00"}, // 2.5V A
    {"3.30", "2.00"}, // 3.3V A
    {"5.00", "00.00"}, // Vlow A
    {"12.00", "00.00"}, // Vhigh A
    {"20.00", "00.00"}  // Vsink A V (VUSB)
};

// Okunacak ADC kanal sayısı
#define TOTAL_ADC_CHANNELS 17 // IN0-IN12 + IN14 + IN15 + Temperature Sensor


const uint32_t selectedChannels[] = {ADC_CHANNEL_5, ADC_CHANNEL_6, ADC_CHANNEL_7, ADC_CHANNEL_8, ADC_CHANNEL_9, ADC_CHANNEL_10, ADC_CHANNEL_11};
const uint8_t totalChannels = sizeof(selectedChannels) / sizeof(selectedChannels[0]);


//SoftI2C i2c1 = {GPIOA, GPIO_PIN_8, GPIOA, GPIO_PIN_9};  // SCL1 = PA8, SDA1 = PA9
//SoftI2C i2c2 = {GPIOC, GPIO_PIN_8, GPIOC, GPIO_PIN_9};   // SCL2 = PC8, SDA2 = PC9
//SoftI2C i2c3 = {GPIOC, GPIO_PIN_6, GPIOC, GPIO_PIN_7};   // SCL3 = PC6, SDA3 = PC7

float PCBTemperature = 0;

const char *channelNames[] = {
    "Curr1", "Curr2", "Curr3", "Curr4", "Curr5", "Curr6",
    "Bus_6V", "3.3V", "Vlow", "Vhigh", "Vusbsink",
    "Vusb", "MCU_3.3V", "4.0V", "2.5V", "1.8V", 	"Temp"
};

#define VREF 3.3 // Referans Voltajı (V)

typedef enum {
    Ch_Curr1,
    Ch_Curr2,
    Ch_Curr3,
    Ch_Curr4,
    Ch_Curr5,
    Ch_Curr6,
    Ch_Bus_6V,
    Ch_3_3V,
    Ch_Vlow,
    Ch_Vhigh,
    Ch_Vusbsink,
    Ch_Vusb,
    Ch_MCU_3_3V,
	Ch_4V,
    Ch_2_5V,
    Ch_1_8V,
    Ch_Temp
} ChannelName;

typedef struct {
    const char *name;  // Kanalın adı
    float Value;
    float nomarlizationfactor; //
} ADC_ChannelData;

ADC_ChannelData channels[TOTAL_ADC_CHANNELS] = {
    {"CurrSink",  0.0 , 2.0}, // 0.01 ohm uzerinden gecen akim x50 Diffopamp ile okunuyor.
    {"Curr3V3",  0.0 , 2.0},
    {"Curr2V5",  0.0 , 2.0},
    {"Curr1V8",  0.0 , 2.0},
    {"CurrVLow",  0.0 , 2.0},
    {"CurrVHigh",  0.0 , 2.0},
    {"Bus_6V",  0.0 , 2.0},  // direnc bolucu normalizasyon
    {"3.3V",  0.0 , 1.0},
    {"Vlow",  0.0 , 2.0},		// direnc bolucu normalizasyon
    {"Vhigh",  0.0 , 16.0},   // direnc bolucu normalizasyon
    {"Vusbsink",  0.0, 6.7},  // direnc bolucu normalizasyon
    {"Vusb",  0.0, 6.7}, // direnc bolucu normalizasyon
    {"MCU_3.3V",  0.0, 1.0},
	{"4.0V",  0.0, 2.0},
    {"2.5V",  0.0, 1.0},
    {"1.8V",  0.0, 1.0},
    {"Temp",  0.0, 1.0}
};



const char *rows_large[] = {
    "1.8V",
    "2.5V",
    "3.3V",
    "V",
    "V",
    "V"
};
const char *rows_small[] = {
    "", // İlk 3 satırda küçük font kullanılmayacak
    "",
    "",
    "Low",
    "High",
    "Sink"
};

float ConvertAdcToTemperature(float vTempSensor) {
    // Sabit değerler (koddan alınmış)
    const int32_t INTERNAL_TEMPSENSOR_V30 = 1430;       // 30°C'deki sensör voltajı (mV)
    const int32_t INTERNAL_TEMPSENSOR_AVGSLOPE = 4300;  // Sensör eğimi (uV/°C)

    // Sıcaklık hesaplama (°C)
    float temperature = ((vTempSensor - INTERNAL_TEMPSENSOR_V30) / INTERNAL_TEMPSENSOR_AVGSLOPE) + 30.0f;

    return temperature;
}

volatile uint32_t UptimeMillis = 0;



uint32_t micros(void)
{
    uint32_t ms;
    uint32_t st;

    do
    {
        ms = UptimeMillis;
        st = SysTick->VAL;
    } while (ms != UptimeMillis);

    return ms * 1000 - st / ((SysTick->LOAD + 1) / 1000);
}


GPIO_PinState prevS1 = GPIO_PIN_SET;
GPIO_PinState prevS2 = GPIO_PIN_SET;
GPIO_PinState prevKey = GPIO_PIN_SET;

GPIO_PinState currS1;
GPIO_PinState currS2;
GPIO_PinState currKey;

volatile bool rotaryPressed = false;




void CheckRotaryButtonPressOnly(void) {
    static GPIO_PinState prevKey = GPIO_PIN_SET;
    static uint32_t lastDebounceTime = 0;
    const uint32_t debounceDelay = 150; // 150ms debounce süresi

    GPIO_PinState currKey = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);

    if (prevKey == GPIO_PIN_SET && currKey == GPIO_PIN_RESET) {
        uint32_t now = HAL_GetTick();
        if (now - lastDebounceTime > debounceDelay) {
            lastDebounceTime = now;
            OnRotaryPressed();  // Menüye giriş/çıkış buradan
            USB_SendMessage("OnRotaryPressed()");
        }
    }

    prevKey = currKey;
}



void StepwiseUpdateDisplay() {
    static uint8_t currentStep = 0;
    static uint8_t row = 0;
    static uint8_t pdoIndex = 0;
    static char buffer[32];

    switch (currentStep) {
        case 0:

            PrepData(channels);
            row = 0;
            break;

        // Satır 0–5
        case 1:  ST7789_WriteString(54, 30 + row * 25, tableData[0].voltage, Font_11x18, YELLOW, BLACK);  break;
        case 2:  ST7789_WriteString(118, 30 + row * 25, tableData[0].current, Font_11x18, YELLOW, BLACK); break;
        case 3:  ST7789_WriteString(178, 30 + row * 25, tableData[0].power,   Font_11x18, YELLOW, BLACK); break;

        case 4:  row = 1; ST7789_WriteString(54, 30 + row * 25, tableData[1].voltage, Font_11x18, YELLOW, BLACK); break;
        case 5:  ST7789_WriteString(118, 30 + row * 25, tableData[1].current, Font_11x18, YELLOW, BLACK); break;
        case 6:  ST7789_WriteString(178, 30 + row * 25, tableData[1].power,   Font_11x18, YELLOW, BLACK); break;

        case 7:  row = 2; ST7789_WriteString(54, 30 + row * 25, tableData[2].voltage, Font_11x18, YELLOW, BLACK); break;
        case 8:  ST7789_WriteString(118, 30 + row * 25, tableData[2].current, Font_11x18, YELLOW, BLACK); break;
        case 9:  ST7789_WriteString(178, 30 + row * 25, tableData[2].power,   Font_11x18, YELLOW, BLACK); break;

        case 10: row = 3; ST7789_WriteString(54, 30 + row * 25, tableData[3].voltage, Font_11x18, YELLOW, BLACK); break;
        case 11: ST7789_WriteString(118, 30 + row * 25, tableData[3].current, Font_11x18, YELLOW, BLACK); break;
        case 12: ST7789_WriteString(178, 30 + row * 25, tableData[3].power,   Font_11x18, YELLOW, BLACK); break;

        case 13: row = 4; ST7789_WriteString(54, 30 + row * 25, tableData[4].voltage, Font_11x18, YELLOW, BLACK); break;
        case 14: ST7789_WriteString(118, 30 + row * 25, tableData[4].current, Font_11x18, YELLOW, BLACK); break;
        case 15: ST7789_WriteString(178, 30 + row * 25, tableData[4].power,   Font_11x18, YELLOW, BLACK); break;

        case 16: row = 5; ST7789_WriteString(54, 30 + row * 25, tableData[5].voltage, Font_11x18, GRAY, BLACK); break;
        case 17: ST7789_WriteString(118, 30 + row * 25, tableData[5].current, Font_11x18, GRAY, BLACK); break;
        case 18: ST7789_WriteString(178, 30 + row * 25, tableData[5].power,   Font_11x18, GRAY, BLACK); break;


    }

    currentStep++;
    if (currentStep > 18)
    	currentStep = 0;
}

/*
void CheckAndDisableOvercurrent() {
    if (Curr3V3 > Currlimit3V3) {
        HAL_GPIO_WritePin(EN1_GPIO_Port, EN1_Pin, GPIO_PIN_RESET);
        DrawVoltageRowsWithOvercurrentCheck();
    }
    if (Curr2V5 > Currlimit2V5) {
        HAL_GPIO_WritePin(EN2_GPIO_Port, EN2_Pin, GPIO_PIN_RESET);
        DrawVoltageRowsWithOvercurrentCheck();
    }
    if (Curr1V8 > Currlimit1V8) {
        HAL_GPIO_WritePin(EN3_GPIO_Port, EN3_Pin, GPIO_PIN_RESET);
        DrawVoltageRowsWithOvercurrentCheck();
    }
    if (CurrVLow > CurrlimitVLow) {
        HAL_GPIO_WritePin(EN4_GPIO_Port, EN4_Pin, GPIO_PIN_RESET);
        DrawVoltageRowsWithOvercurrentCheck();
    }
    if (CurrVHigh > CurrlimitVHigh) {
        HAL_GPIO_WritePin(EN5_GPIO_Port, EN5_Pin, GPIO_PIN_RESET);
        DrawVoltageRowsWithOvercurrentCheck();
    }

}
*/



bool channelEnabled[5] = {true, true, true, true, true};         // Gerçek kanal durumu
bool prevChannelEnabled[5] = {false, false, false, false, false}; // Önceki durum (ilk turda farklı olacak)

void CheckAndDisableOvercurrent() {
    int y_position;

    // 3.3V - index 2
    y_position = 30 + 2 * 25;
    if (Curr3V3 > Currlimit3V3) {
        HAL_GPIO_WritePin(EN1_GPIO_Port, EN1_Pin, GPIO_PIN_RESET);
        channelEnabled[2] = false;
    }
    if (channelEnabled[2] != prevChannelEnabled[2]) {
        //ST7789_DrawFilledCircle(240, y_position + 8, 6, channelEnabled[2] ? GREEN : RED);
        DrawChannelCircle(2, channelEnabled[2]);
        prevChannelEnabled[2] = channelEnabled[2];
    }

    // 2.5V - index 1
    y_position = 30 + 1 * 25;
    if (Curr2V5 > Currlimit2V5) {
        HAL_GPIO_WritePin(EN2_GPIO_Port, EN2_Pin, GPIO_PIN_RESET);
        channelEnabled[1] = false;
    }
    if (channelEnabled[1] != prevChannelEnabled[1]) {
        //ST7789_DrawFilledCircle(240, y_position + 8, 6, channelEnabled[1] ? GREEN : RED);
        DrawChannelCircle(1, channelEnabled[1]);
        prevChannelEnabled[1] = channelEnabled[1];
    }

    // 1.8V - index 0
    y_position = 30 + 0 * 25;
    if (Curr1V8 > Currlimit1V8) {
        HAL_GPIO_WritePin(EN3_GPIO_Port, EN3_Pin, GPIO_PIN_RESET);
        channelEnabled[0] = false;
    }
    if (channelEnabled[0] != prevChannelEnabled[0]) {
        //ST7789_DrawFilledCircle(240, y_position + 8, 6, channelEnabled[0] ? GREEN : RED);
    	DrawChannelCircle(0, channelEnabled[0]);
        prevChannelEnabled[0] = channelEnabled[0];
    }

    // VLow - index 3
    y_position = 30 + 3 * 25;
    if (CurrVLow > CurrlimitVLow) {
        HAL_GPIO_WritePin(EN4_GPIO_Port, EN4_Pin, GPIO_PIN_RESET);
        channelEnabled[3] = false;
    }
    if (channelEnabled[3] != prevChannelEnabled[3]) {
    	//ST7789_DrawFilledCircle(240, y_position + 8, 6, channelEnabled[3] ? GREEN : RED);
    	DrawChannelCircle(3, channelEnabled[3]);
        prevChannelEnabled[3] = channelEnabled[3];
    }

    // VHigh - index 4
    y_position = 30 + 4 * 25;
    if (CurrVHigh > CurrlimitVHigh) {
        HAL_GPIO_WritePin(EN5_GPIO_Port, EN5_Pin, GPIO_PIN_RESET);
        channelEnabled[4] = false;
    }
    if (channelEnabled[4] != prevChannelEnabled[4]) {
    	//ST7789_DrawFilledCircle(240, y_position + 8, 6, channelEnabled[4] ? GREEN : RED);
    	DrawChannelCircle(4, channelEnabled[4]);
        prevChannelEnabled[4] = channelEnabled[4];
    }


}


void ManuArrowsStepwise(void)
{
    static uint8_t step = 0;
    static uint32_t caseElapsedTime[10] = {0}; // Microseconds spent in each case
    uint32_t startTime, endTime;

    int y_position, xOffset, arrowOffset, centerY, circleCenterX, circleRadius, i;

    startTime = micros(); // Record start time

    switch (step)
    {
        case 0:
            // Draw filled circle for VLow channel (row 3)
            y_position = 30 + 3 * 25;
            xOffset = 10;
            circleCenterX = 235 + xOffset;
            centerY = y_position + 8;
            circleRadius = 6;
            ST7789_DrawFilledCircle(circleCenterX, centerY, circleRadius, channelEnabled[3] ? GREEN : RED);
            break;
        case 1:
            // Draw upward arrow for VLow channel
            y_position = 30 + 3 * 25;
            arrowOffset = 10 + 6;
            centerY = y_position + 8;
            ST7789_DrawFilledTriangle(251 + arrowOffset, centerY - 7,
                                     246 + arrowOffset, centerY - 2,
                                     256 + arrowOffset, centerY - 2, WHITE);
            break;
        case 2:
            // Draw downward arrow for VLow channel
            y_position = 30 + 3 * 25;
            arrowOffset = 10 + 6;
            centerY = y_position + 8;
            ST7789_DrawFilledTriangle(251 + arrowOffset, centerY + 7,
                                     246 + arrowOffset, centerY + 2,
                                     256 + arrowOffset, centerY + 2, WHITE);
            break;
        case 3:
            // Draw thick blue rectangle around VLow channel (circle)
            y_position = 30 + 3 * 25;
            xOffset = 10;
            circleCenterX = 235 + xOffset;
            circleRadius = 6;
            for (i = 0; i < 2; i++) {
                ST7789_DrawRectangle(circleCenterX - circleRadius - 2 - i, y_position - 2 - i,
                                     circleCenterX + circleRadius + 2 + i, y_position + 18 + i, BLUE);
            }
            break;
        case 4:
            // Draw thick blue rectangle around VLow channel (arrows)
            y_position = 30 + 3 * 25;
            arrowOffset = 10 + 6;
            for (i = 0; i < 2; i++) {
                ST7789_DrawRectangle(242 + arrowOffset - i, y_position - 2 - i,
                                     260 + arrowOffset + i, y_position + 18 + i, BLUE);
            }
            break;
        case 5:
            // Draw filled circle for VHigh channel (row 4)
            y_position = 30 + 4 * 25;
            xOffset = 10;
            circleCenterX = 240 + xOffset;
            centerY = y_position + 8;
            circleRadius = 6;
            ST7789_DrawFilledCircle(circleCenterX, centerY, circleRadius, channelEnabled[4] ? GREEN : RED);
            break;
        case 6:
            // Draw upward arrow for VHigh channel
            y_position = 30 + 4 * 25;
            arrowOffset = 10 + 6;
            centerY = y_position + 8;
            ST7789_DrawFilledTriangle(251 + arrowOffset, centerY - 7,
                                     246 + arrowOffset, centerY - 2,
                                     256 + arrowOffset, centerY - 2, WHITE);
            break;
        case 7:
            // Draw downward arrow for VHigh channel
            y_position = 30 + 4 * 25;
            arrowOffset = 10 + 6;
            centerY = y_position + 8;
            ST7789_DrawFilledTriangle(251 + arrowOffset, centerY + 7,
                                     246 + arrowOffset, centerY + 2,
                                     256 + arrowOffset, centerY + 2, WHITE);
            break;
        case 8:
            // Draw thick blue rectangle around VHigh channel (circle)
            y_position = 30 + 4 * 25;
            xOffset = 10;
            circleCenterX = 240 + xOffset;
            circleRadius = 6;
            for (i = 0; i < 2; i++) {
                ST7789_DrawRectangle(circleCenterX - circleRadius - 2 - i, y_position - 2 - i,
                                     circleCenterX + circleRadius + 2 + i, y_position + 18 + i, BLUE);
            }
            break;
        case 9:
            // Draw thick blue rectangle around VHigh channel (arrows)
            y_position = 30 + 4 * 25;
            arrowOffset = 10 + 6;
            for (i = 0; i < 2; i++) {
                ST7789_DrawRectangle(242 + arrowOffset - i, y_position - 2 - i,
                                     260 + arrowOffset + i, y_position + 18 + i, BLUE);
            }
            break;
        default:
            step = 0;
            return;
    }

    endTime = micros(); // Record end time

    // Store elapsed time for this case
    if (step < 10)
        caseElapsedTime[step] = endTime - startTime;

    step++;
    if (step > 9)
        step = 0;
}


void ManuArrows()
{
	//#################################################
		int y_position = 30 + 3 * 25;

    	int xOffset = 10;
    	int arrowOffset = xOffset + 6;
    	int centerY = y_position + 8;

    	// Daire merkezi ve çerçeve boyutu
    	int circleCenterX = 235 + xOffset;
    	int circleRadius = 6;

    	// Daire çizimi


    	ST7789_DrawFilledCircle(circleCenterX, centerY, circleRadius, channelEnabled[3] ? GREEN : RED);

    	prevChannelEnabled[3] = channelEnabled[3];

    	// Yukarı ok
    	ST7789_DrawFilledTriangle(251 + arrowOffset, centerY - 7,
    	                          246 + arrowOffset, centerY - 2,
    	                          256 + arrowOffset, centerY - 2, WHITE);

    	// Aşağı ok
    	ST7789_DrawFilledTriangle(251 + arrowOffset, centerY + 7,
    	                          246 + arrowOffset, centerY + 2,
    	                          256 + arrowOffset, centerY + 2, WHITE);

    	// Daire için kalın mavi çerçeve (2 piksel)
    	for (int i = 0; i < 2; i++) {
    	    ST7789_DrawRectangle(circleCenterX - circleRadius - 2 - i, y_position - 2 - i,
    	                         circleCenterX + circleRadius + 2 + i, y_position + 18 + i, BLUE);
    	}

    	// Oklar için kalın mavi çerçeve (daha sağda)
    	for (int i = 0; i < 2; i++) {
    	    ST7789_DrawRectangle(242 + arrowOffset - i, y_position - 2 - i,
    	                         260 + arrowOffset + i, y_position + 18 + i, BLUE);
    	}

    	//#################################################





    	//#################################################
    	y_position = 30 + 4 * 25;
    	xOffset = 10;
    	arrowOffset = xOffset + 6;
    	centerY = y_position + 8;

    	// Daire merkezi ve çerçeve boyutu
    	circleCenterX = 240 + xOffset;
    	circleRadius = 6;

    	// Daire çizimi
    	ST7789_DrawFilledCircle(circleCenterX, centerY, circleRadius, channelEnabled[4] ? GREEN : RED);

    	// Yukarı ok
    	ST7789_DrawFilledTriangle(251 + arrowOffset, centerY - 7,
    	                          246 + arrowOffset, centerY - 2,
    	                          256 + arrowOffset, centerY - 2, WHITE);

    	// Aşağı ok
    	ST7789_DrawFilledTriangle(251 + arrowOffset, centerY + 7,
    	                          246 + arrowOffset, centerY + 2,
    	                          256 + arrowOffset, centerY + 2, WHITE);

    	// Daire için kalın mavi çerçeve (2 piksel)
    	for (int i = 0; i < 2; i++) {
    	    ST7789_DrawRectangle(circleCenterX - circleRadius - 2 - i, y_position - 2 - i,
    	                         circleCenterX + circleRadius + 2 + i, y_position + 18 + i, BLUE);
    	}

    	// Oklar için kalın mavi çerçeve (daha sağda)
    	for (int i = 0; i < 2; i++) {
    	    ST7789_DrawRectangle(242 + arrowOffset - i, y_position - 2 - i,
    	                         260 + arrowOffset + i, y_position + 18 + i, BLUE);
    	}
    	//#################################################
}
void DrawChannelStatusCircles(void) {
    for (int i = 0; i < 6; i++) {
        int y = 30 + i * 25; // Satırların merkez yüksekliği (tablonla uyumlu)

        bool channelOn = false;

        switch (i) {
            case 0: channelOn = (HAL_GPIO_ReadPin(EN3_GPIO_Port, EN3_Pin) == GPIO_PIN_SET); break; // 1.8V
            case 1: channelOn = (HAL_GPIO_ReadPin(EN2_GPIO_Port, EN2_Pin) == GPIO_PIN_SET); break; // 2.5V
            case 2: channelOn = (HAL_GPIO_ReadPin(EN1_GPIO_Port, EN1_Pin) == GPIO_PIN_SET); break; // 3.3V
            case 3: channelOn = (HAL_GPIO_ReadPin(EN4_GPIO_Port, EN4_Pin) == GPIO_PIN_SET); break; // VLow
            case 4: channelOn = (HAL_GPIO_ReadPin(EN5_GPIO_Port, EN5_Pin) == GPIO_PIN_SET); break; // VHigh
            case 5: channelOn = true; break; // VSink sabit açık (GPIO ile kontrol edilmiyor)
        }

        uint16_t color = channelOn ? GREEN : RED;
        ST7789_DrawFilledCircle(224, y + 8, 4, color); // x:224 y:38/63/88... çap:8px
    }
}


void DrawVoltageRowsWithOvercurrentCheck(void) {

    int y_position = 25;

    for (int i = 0; i < 6; i++) {
        y_position = 25 + (i * 25);

        bool overcurrent = false;

        // Check overcurrent condition for each row
        if ((i == 2 && Curr3V3 > Currlimit3V3) ||
            (i == 1 && Curr2V5 > Currlimit2V5) ||
            (i == 0 && Curr1V8 > Currlimit1V8) ||
			(i == 5 && CurrVSink > CurrlimitVSink) ||
            (i == 3 && CurrVLow > CurrlimitVLow) ||
            (i == 4 && CurrVHigh > CurrlimitVHigh)) {
            overcurrent = true;
        }

        if (overcurrent) {
            uint16_t textColor = RED;

            ST7789_WriteString(0, y_position + 5, rows_large[i], Font_11x18, textColor, BLACK);

            if (strlen(rows_small[i]) > 0) {
                ST7789_WriteString(12, y_position + 10, rows_small[i], Font_7x10, textColor, BLACK);
            }

            ST7789_DrawLine(0, y_position + 25, 230, y_position + 25, WHITE);
        }
    }
}


void DisplayTableWithGrid() {

	DrawTableWithGrid(false);


}


void DrawTableWithGrid(bool isMainMenu)
{
    // Clear the screen
    ST7789_Fill_Color(BLACK);

    // Display table headers (büyük ve küçük harf kombinasyonu, aynı hizalama)
    ST7789_WriteString(60, 3, "V", Font_11x18, LIGHTBLUE, BLACK);  // "V" büyük
    ST7789_WriteString(70, 10, "olt", Font_7x10, LIGHTBLUE, BLACK); // "olt" büyük harfin altına hizalı

    ST7789_WriteString(120, 3, "A", Font_11x18, LIGHTBLUE, BLACK); // "A" büyük
    ST7789_WriteString(130, 10, "mper", Font_7x10, LIGHTBLUE, BLACK); // "mper" büyük harfin altına hizalı

    if (!isMainMenu)
    {
        ST7789_WriteString(180, 3, "W", Font_11x18, LIGHTBLUE, BLACK);  // "W" büyük
        ST7789_WriteString(190, 10, "att", Font_7x10, LIGHTBLUE, BLACK); // "att" büyük harfin altına hizalı
    }




    // İlk yatay çizgiyi çizin
    int y_position = 25;
    if (!isMainMenu)
    {
        ST7789_DrawLine(0, y_position, 230, y_position, WHITE); // İlk çizgi
    }
    else
    {
        ST7789_DrawLine(0, y_position, 170, y_position, WHITE); // İlk çizgi
    }


    // Display rows dynamically with grid lines
    for (int i = 0; i < 6; i++) {
        y_position = 25 + (i * 25); // Yatay çizgiler arası mesafeyi ayarla

        // Volt kısmını büyük fontla yaz
        ST7789_WriteString(0, y_position + 5, rows_large[i], Font_11x18, LIGHTBLUE, BLACK);

        // Eğer küçük bir ek metin varsa, onu küçük fontla yaz
        if (strlen(rows_small[i]) > 0) {
            ST7789_WriteString(12, y_position + 10, rows_small[i], Font_7x10, LIGHTBLUE, BLACK);
        }

        // Draw horizontal line below the row
        if (!isMainMenu)
        {
        	ST7789_DrawLine(0, y_position + 25, 230, y_position + 25, WHITE); // Full-width line
        }
        else
        {
        	ST7789_DrawLine(0, y_position + 25, 170, y_position + 25, WHITE); // Full-width line
        }




    }

    // Draw vertical lines to create grid
    ST7789_DrawLine(50, 0, 50, 185, WHITE);
    ST7789_DrawLine(110, 0, 110, 185, WHITE);
    ST7789_DrawLine(170, 0, 170, 185, WHITE);
    if (!isMainMenu)
    {
    	ST7789_DrawLine(230, 0, 230, 185, WHITE);
    	ST7789_WriteString(2, 4, "Menu", Font_11x18, WHITE, BLACK);// Menu STring
    }




}


void WriteExtraInfoStepByStep(void)
{
    static uint8_t step = 0;
    char buffer[16];

    static uint8_t pdo_raw[12]; // PDO register içerikleri



    switch (step)
    {
        case 0:
            snprintf(buffer, sizeof(buffer), "Vusb:%.2fV", channels[11].Value);
            ST7789_WriteString(235, 2, buffer, Font_7x10, WHITE, BLACK);
            break;

        case 1:
            snprintf(buffer, sizeof(buffer), "Temp:%.2fC", PCBTemperature);
            ST7789_WriteString(235, 15, buffer, Font_7x10, WHITE, BLACK);
            break;

        case 2:
            snprintf(buffer, sizeof(buffer), "5.5V:%.2fV", channels[6].Value);
            ST7789_WriteString(235, 28, buffer, Font_7x10, WHITE, BLACK);
            break;

        case 3:
            snprintf(buffer, sizeof(buffer), "4V  :%.2fV", channels[13].Value);
            ST7789_WriteString(235, 41, buffer, Font_7x10, WHITE, BLACK);
            break;


        case 4:
             for (int i = 0; i < 4; i++) {
                 pdo_raw[i] = read_register(REG_DPM_SNK_PDO1_0 + i);
             }
             break;

         case 5:
             for (int i = 4; i < 8; i++) {
                 pdo_raw[i] = read_register(REG_DPM_SNK_PDO1_0 + i);
             }
             break;

         case 6:
             for (int i = 8; i < 12; i++) {
                 pdo_raw[i] = read_register(REG_DPM_SNK_PDO1_0 + i);
             }
             break;

        case 7:
        {
            uint32_t val = (pdo_raw[3] << 24) | (pdo_raw[2] << 16) | (pdo_raw[1] << 8) | pdo_raw[0];
            uint32_t voltage = ((val >> 10) & 0x3FF) * 50;
            uint32_t current = (val & 0x3FF) * 10;
            snprintf(buffer, sizeof(buffer), "%luV @%luA", voltage / 1000, current / 1000);
            ST7789_WriteString(235, 70, buffer, Font_7x10, WHITE, BLACK);
            break;
        }

        case 8:
        {
            uint32_t val = (pdo_raw[7] << 24) | (pdo_raw[6] << 16) | (pdo_raw[5] << 8) | pdo_raw[4];
            uint32_t voltage = ((val >> 10) & 0x3FF) * 50;
            uint32_t current = (val & 0x3FF) * 10;
            snprintf(buffer, sizeof(buffer), "%luV @%luA", voltage / 1000, current / 1000);
            ST7789_WriteString(235, 82, buffer, Font_7x10, WHITE, BLACK);
            break;
        }

        case 9:
        {
            uint32_t val = (pdo_raw[11] << 24) | (pdo_raw[10] << 16) | (pdo_raw[9] << 8) | pdo_raw[8];
            uint32_t voltage = ((val >> 10) & 0x3FF) * 50;
            uint32_t current = (val & 0x3FF) * 10;
            snprintf(buffer, sizeof(buffer), "%luV @%luA", voltage / 1000, current / 1000);
            ST7789_WriteString(235, 94, buffer, Font_7x10, WHITE, BLACK);
            break;
        }

        case 10:
        {
        	PCBTemperature = read_temp(0);
            break;
        }



    }

    step = (step + 1) % 11;
}



void WriteMenuString()
{
	int row = 0;
	char buffer[16];


	//1.8V
	snprintf(buffer, sizeof(buffer), "%.2f", Volt1V8);
	ST7789_WriteString(54, 30 + row * 25, buffer, Font_11x18, WHITE, BLACK);
	snprintf(buffer, sizeof(buffer), "%.2f", Currlimit1V8);
	ST7789_WriteString(118, 30 + row * 25, buffer , Font_11x18, GREEN, BLACK);
	//2.5v
	snprintf(buffer, sizeof(buffer), "%.2f", Volt2V5);
	row = 1; ST7789_WriteString(54, 30 + row * 25, buffer, Font_11x18, WHITE, BLACK);
	snprintf(buffer, sizeof(buffer), "%.2f", Currlimit2V5);
	ST7789_WriteString(118, 30 + row * 25, buffer, Font_11x18, GREEN, BLACK);
	//3.3v
	snprintf(buffer, sizeof(buffer), "%.2f", Volt3V3);
	row = 2; ST7789_WriteString(54, 30 + row * 25,buffer , Font_11x18, WHITE, BLACK);
	snprintf(buffer, sizeof(buffer), "%.2f", Currlimit3V3);
	ST7789_WriteString(118, 30 + row * 25,buffer , Font_11x18, GREEN, BLACK);
	//Vlow
	snprintf(buffer, sizeof(buffer), "%.2f", VoltVLow);
	row = 3; ST7789_WriteString(54, 30 + row * 25,buffer , Font_11x18, GREEN, BLACK);
	snprintf(buffer, sizeof(buffer), "%.2f", CurrlimitVLow);
	ST7789_WriteString(118, 30 + row * 25, buffer, Font_11x18, GREEN, BLACK);
    // Vhigh
	snprintf(buffer, sizeof(buffer), "%.2f", VoltVHigh);
	row = 4; ST7789_WriteString(54, 30 + row * 25, buffer, Font_11x18, GREEN, BLACK);
	snprintf(buffer, sizeof(buffer), "%.2f", CurrlimitVHigh);
	ST7789_WriteString(118, 30 + row * 25,buffer , Font_11x18, GREEN, BLACK);


	//row = 5; ST7789_WriteString(58, 30 + row * 25, VoltVSink, Font_11x18, GRAY, BLACK); break;
	//ST7789_WriteString(118, 30 + row * 25, CurrlimitVSink, Font_11x18, GRAY, BLACK); break;
}

#define NUM_SAMPLES 1

int selectedRow = 0;
int selectedColumn = 1;
bool menuAdjustMode = false;

typedef enum {
    MENU_MAIN,
	MENU_MAIN_AVTIVE,
    MENU_LIMIT_SET,
	MENU_LIMIT_SET_ACTIVE
} MenuState;

volatile MenuState menuState = MENU_MAIN;

void RedrawAllTableRows(void) {
    if (!(menuState == MENU_LIMIT_SET_ACTIVE || menuState == MENU_LIMIT_SET)) return;

    for (int i = 0; i < 6; i++) {
        int y_position = 25 + (i * 25);

        char voltStr[16], currStr[16];
        uint16_t labelColor = LIGHTBLUE;
        uint16_t voltColor = WHITE;
        uint16_t currColor = WHITE;

        if (selectedRow == i) {
            if (selectedColumn == 1) voltColor = menuAdjustMode ? RED : BLUE;
            else if (selectedColumn == 2) currColor = menuAdjustMode ? RED : BLUE;
        }

        switch (i) {
            case 0: snprintf(voltStr, sizeof(voltStr), "%.2f", Volt1V8);   snprintf(currStr, sizeof(currStr), "%.2f", Currlimit1V8); break;
            case 1: snprintf(voltStr, sizeof(voltStr), "%.2f", Volt2V5);   snprintf(currStr, sizeof(currStr), "%.2f", Currlimit2V5); break;
            case 2: snprintf(voltStr, sizeof(voltStr), "%.2f", Volt3V3);   snprintf(currStr, sizeof(currStr), "%.2f", Currlimit3V3); break;
            case 3: snprintf(voltStr, sizeof(voltStr), "%.2f", VoltVLow);  snprintf(currStr, sizeof(currStr), "%.2f", CurrlimitVLow); break;
            case 4: snprintf(voltStr, sizeof(voltStr), "%-5.2f", VoltVHigh); snprintf(currStr, sizeof(currStr), "%.2f", CurrlimitVHigh); break;
            case 5: snprintf(voltStr, sizeof(voltStr), "%-5.2f", VoltVSink); snprintf(currStr, sizeof(currStr), "%.2f", CurrlimitVSink); break;
        }

        ST7789_WriteString(0, y_position + 5, rows_large[i], Font_11x18, labelColor, BLACK);

        if (strlen(rows_small[i]) > 0) {
            ST7789_WriteString(12, y_position + 10, rows_small[i], Font_7x10, labelColor, BLACK);
        }

        ST7789_WriteString(54, y_position + 5, voltStr, Font_11x18, voltColor, BLACK);
        ST7789_WriteString(118, y_position + 5, currStr, Font_11x18, currColor, BLACK);

        //ST7789_DrawLine(0, y_position + 25, 170, y_position + 25, WHITE);
    }

    // Exit alanı
    uint16_t exitColor = (selectedRow == 6) ? (menuAdjustMode ? RED : BLUE) : WHITE;
    ST7789_WriteString(2, 4, "Exit", Font_11x18, exitColor, BLACK);
}



void PrintMenuSelection(void) {


	 if ( (menuState == MENU_LIMIT_SET_ACTIVE || menuState == MENU_LIMIT_SET)) return;

	static int prevSelectedRow = -1;
	static int prevSelectedColumn = -1;

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Menu Selection -> Row: %d, Column: %d\r\n", selectedRow, selectedColumn);
    USB_SendMessage(buffer);

    DrawChannelCircleFrame(0, WHITE);
    DrawChannelCircleFrame(1, WHITE);
    DrawChannelCircleFrame(2, WHITE);
    DrawChannelCircleFrame(3, WHITE);
    DrawChannelCircleFrame(4, WHITE);
    DrawChannelArrowFrame(3, WHITE);
    DrawChannelArrowFrame(4, WHITE);

     if (selectedColumn == 1 && selectedRow < 5)
     {
    	 DrawChannelCircleFrame(selectedRow, BLUE);
     }
     else if (selectedColumn == 2)
     {
    	 if (selectedRow == 6)
    	 {
    		 //ST7789_WriteString(2, 4, "Menu", Font_11x18, BLUE, BLACK);// Menu STring
    	 }
    	 else  if (selectedRow == 3 || selectedRow == 4)
    	 {
        	 DrawChannelArrowFrame(selectedRow, BLUE);
    	 }
       	 else  if (selectedRow <3)
        	 {
       		 	 DrawChannelCircleFrame(selectedRow, BLUE);
        	 }
     }


     // Yeni seçimleri sakla
      prevSelectedRow = selectedRow;
      prevSelectedColumn = selectedColumn;

}


void SetVadjLVoltage(float value) {
    // Calibration points
    const float Vref1 = 0.50f;   // Voltage at calibration point 1
    const int   DACref1 = 3975;  // DAC code corresponding to Vref1

    const float Vref2 = 5.00f;   // Voltage at calibration point 2
    const int   DACref2 = 340;   // DAC code corresponding to Vref2

    // Calculate slope and offset for linear mapping
    float m = (DACref2 - DACref1) / (Vref2 - Vref1);  // slope
    float b = DACref1 - m * Vref1;                    // offset

    // Compute DAC value for the requested voltage
    float dacValueF = m * value + b;

    // Clamp to valid DAC range (12-bit: 0..4095)
    if (dacValueF < 0) dacValueF = 0;
    if (dacValueF > 4095) dacValueF = 4095;

    // Convert to integer with rounding
    uint16_t dacValue = (uint16_t)(dacValueF + 0.5f);

    // Write to DAC
    set_dacVLow(dacValue);
}

void SetVadjHVoltage(float value) {
    // Calibration points
    const float Vref1 = 2.50f;   // Voltage at calibration point 1
    const int   DACref1 = 3884;  // DAC code corresponding to Vref1

    const float Vref2 = 32.00f;   // Voltage at calibration point 2
    const int   DACref2 = 218;   // DAC code corresponding to Vref2

    // Calculate slope and offset for linear mapping
    float m = (DACref2 - DACref1) / (Vref2 - Vref1);  // slope
    float b = DACref1 - m * Vref1;                    // offset

    // Compute DAC value for the requested voltage
    float dacValueF = m * value + b;

    // Clamp to valid DAC range (12-bit: 0..4095)
    if (dacValueF < 0) dacValueF = 0;
    if (dacValueF > 4095) dacValueF = 4095;

    // Convert to integer with rounding
    uint16_t dacValue = (uint16_t)(dacValueF + 0.5f);

    // Write to DAC
    set_dacVHigh(dacValue);
}

uint16_t dacValueVlow;
uint16_t dacValueVhigh;

void OnRotaryUP(bool Fast) {



    float step = Fast ? 0.5f : 0.01f;

    if (menuAdjustMode) {
        if (selectedColumn == 1) { // Voltaj ayarı
            if (selectedRow == 3) {
                VoltVLow += step;
                if (VoltVLow > MaxVoltVLow) VoltVLow = MaxVoltVLow;
                SetVadjLVoltage(VoltVLow);

            }
            else if (selectedRow == 4) {
                VoltVHigh += step;
                if (VoltVHigh > MaxVoltVHigh) VoltVHigh = MaxVoltVHigh;
                SetVadjHVoltage(VoltVHigh);

            }
            else if (selectedRow == 5) {
                VoltVSink += step;
            	if (VoltVSink > MaxVoltVSink) VoltVSink = MaxVoltVSink;
            }
        } else if (selectedColumn == 2) { // Akım limiti ayarı
            if (selectedRow == 0) {
                Currlimit1V8 += step;
                if (Currlimit1V8 > MaxCurrlimit1V8) Currlimit1V8 = MaxCurrlimit1V8;
            }
            else if (selectedRow == 1) {
                Currlimit2V5 += step;
                if (Currlimit2V5 > MaxCurrlimit2V5) Currlimit2V5 = MaxCurrlimit2V5;
            }
            else if (selectedRow == 2) {
                Currlimit3V3 += step;
                if (Currlimit3V3 > MaxCurrlimit3V3) Currlimit3V3 = MaxCurrlimit3V3;
            }
            else if (selectedRow == 3) {
                CurrlimitVLow += step;
                if (CurrlimitVLow > MaxCurrlimitVLow) CurrlimitVLow = MaxCurrlimitVLow;
            }
            else if (selectedRow == 4) {
                CurrlimitVHigh += step;
                if (CurrlimitVHigh > MaxCurrlimitVHigh) CurrlimitVHigh = MaxCurrlimitVHigh;
            }
            else if (selectedRow == 5) {
                CurrlimitVSink += step;
                if (CurrlimitVSink > MaxCurrlimitVSink) CurrlimitVSink = MaxCurrlimitVSink;
            }
        }
    } else {
        selectedRow--;
        if (selectedRow < 0) {
            if (selectedColumn == 2) {
                selectedColumn = 1;
                selectedRow = 6; // Exit'e geç
            } else if (selectedColumn == 1) {
                selectedColumn = 2;
                selectedRow = 5;
            }
        }
    }

    PrintMenuSelection();
}

void OnRotaryDown(bool Fast) {


    float step = Fast ? 0.5f : 0.01f;

    if (menuAdjustMode) {
        if (selectedColumn == 1) { // Voltaj ayarı
            if (selectedRow == 3) {
                VoltVLow -= step;
                if (VoltVLow < MinVoltVLow) VoltVLow = MinVoltVLow;
                SetVadjLVoltage(VoltVLow);

            }
            else if (selectedRow == 4) {
                VoltVHigh -= step;
                if (VoltVHigh < MinVoltVHigh) VoltVHigh = MinVoltVHigh;
                SetVadjHVoltage(VoltVHigh);

            }
            else if (selectedRow == 5) {
                VoltVSink -= step;
                if (VoltVSink < 0.0f) VoltVSink = 0.0f;
            }
        } else if (selectedColumn == 2) { // Akım limiti ayarı
            if (selectedRow == 0) {
                Currlimit1V8 -= step;
                if (Currlimit1V8 < 0.0f) Currlimit1V8 = 0.0f;
            }
            else if (selectedRow == 1) {
                Currlimit2V5 -= step;
                if (Currlimit2V5 < 0.0f) Currlimit2V5 = 0.0f;
            }
            else if (selectedRow == 2) {
                Currlimit3V3 -= step;
                if (Currlimit3V3 < 0.0f) Currlimit3V3 = 0.0f;
            }
            else if (selectedRow == 3) {
                CurrlimitVLow -= step;
                if (CurrlimitVLow < 0.0f) CurrlimitVLow = 0.0f;
            }
            else if (selectedRow == 4) {
                CurrlimitVHigh -= step;
                if (CurrlimitVHigh < 0.0f) CurrlimitVHigh = 0.0f;
            }
            else if (selectedRow == 5) {
                CurrlimitVSink -= step;
                if (CurrlimitVSink < 0.0f) CurrlimitVSink = 0.0f;
            }
        }
    } else {
        selectedRow++;
        if ((selectedColumn == 1 && selectedRow > 5)) {
            selectedColumn = 2;
            selectedRow = 0;
        } else if (selectedColumn == 2 && selectedRow > 5) {
            selectedRow = 6; // Exit'e geç
        } else if (selectedRow > 6) {
            selectedColumn = 1;
            selectedRow = 0;
        }
    }

    PrintMenuSelection();
}




volatile int selectedFrame = -1;  // -1: hiçbir çerçeve seçili değil


void OnRotaryPressed(void) {

    rotaryPressed = true;

    switch (menuState)
        {

			case MENU_MAIN:
				selectedRow = 3;
				selectedColumn = 1;
				menuAdjustMode = false;
				menuState = MENU_LIMIT_SET;
				DrawTableWithGrid(true);
				return;
				break;


			case MENU_LIMIT_SET:

				if (selectedRow == 6 )
				{
					selectedColumn = 0;
					DrawTableWithGrid(false);

					for (int i = 0; i < 5; i++) {
						  prevChannelEnabled[i] = !channelEnabled[i]; // Zorla fark yarat
					  }
					menuState = MENU_MAIN;
					DrawMenuArrowsAndCirclesInit();

					return;
				}
				break;

			default:  break;
        }


    menuAdjustMode = !menuAdjustMode;


}


void GetADCValuesWithStatsFast() {
    uint32_t sum[TOTAL_ADC_CHANNELS] = {0};
    uint16_t minVal[TOTAL_ADC_CHANNELS];
    uint16_t maxVal[TOTAL_ADC_CHANNELS];

    // İlk değerleri max/min başlat
    for (uint8_t i = 0; i < TOTAL_ADC_CHANNELS; i++) {
        minVal[i] = 0xFFFF;
        maxVal[i] = 0;
    }

    for (uint16_t j = 0; j < NUM_SAMPLES; j++) {
        for (uint8_t i = 0; i < TOTAL_ADC_CHANNELS; i++) {

            HAL_ADC_Start(&hadc);
            if (HAL_ADC_PollForConversion(&hadc, 10000) != HAL_OK) {
                continue; // timeout olduysa bu örneği atla
            }

            uint16_t adcValue = HAL_ADC_GetValue(&hadc);

            sum[i] += adcValue;
            if (adcValue < minVal[i]) minVal[i] = adcValue;
            if (adcValue > maxVal[i]) maxVal[i] = adcValue;
        }
    }

    // Ortalama ve sonuçları float'a dönüştür
    for (uint8_t i = 0; i < TOTAL_ADC_CHANNELS; i++) {
        float avg = (sum[i] / (float)NUM_SAMPLES) * VREF / 4095.0f * channels[i].nomarlizationfactor;
        float vmin = (minVal[i] * VREF / 4095.0f) * channels[i].nomarlizationfactor;
        float vmax = (maxVal[i] * VREF / 4095.0f) * channels[i].nomarlizationfactor;

        channels[i].Value = avg;

        if (i == 1)
			Curr3V3 = avg;
		else if (i == 2)
			Curr2V5 = avg;
		else if (i == 3)
			Curr1V8 = avg;
		else if (i == 4)
			CurrVLow = avg;
		else if (i == 5)
			CurrVHigh = avg;
        // Eğer USB mesaj gerekiyorsa sadece burada yazdır
        // snprintf veya USB_SendMessage buraya konabilir
    }
}


void PrepData(ADC_ChannelData channels[]) {


	float SinkPowerW = channels[0].Value * channels[10].Value ;	// CurrSink * Vusbsink;
	snprintf(tableData[5].voltage, sizeof(tableData[5].voltage), "%.2f", channels[10].Value);
	snprintf(tableData[5].current, sizeof(tableData[5].current), "%.2f", channels[0].Value);
	snprintf(tableData[5].power, sizeof(tableData[5].power), "%.2f", SinkPowerW);


	float  PowerW3V3 = channels[1].Value * channels[7].Value ;	// Curr3V3 * 3.3V;
	snprintf(tableData[2].voltage, sizeof(tableData[2].voltage), "%.2f", channels[7].Value);
	snprintf(tableData[2].current, sizeof(tableData[2].current), "%.2f", channels[1].Value);
	snprintf(tableData[2].power, sizeof(tableData[2].power), "%.2f", PowerW3V3);

	float  PowerW4V0 = channels[13].Value ;	// Curr4V0 *4.0V;

	float  PowerW2V5 = channels[2].Value * channels[14].Value ;	// Curr2V5 * 2.5V;
	snprintf(tableData[1].voltage, sizeof(tableData[1].voltage), "%.2f", channels[14].Value);
	snprintf(tableData[1].current, sizeof(tableData[1].current), "%.2f", channels[2].Value);
	snprintf(tableData[1].power, sizeof(tableData[1].power), "%.2f", PowerW2V5);

	float  PowerW1V8 = channels[3].Value * channels[15].Value ;	// Curr1V8 * 1.8V;
	snprintf(tableData[0].voltage, sizeof(tableData[0].voltage), "%.2f", channels[15].Value);
	snprintf(tableData[0].current, sizeof(tableData[0].current), "%.2f", channels[3].Value);
	snprintf(tableData[0].power, sizeof(tableData[0].power), "%.2f", PowerW1V8);

	float  PowerVlow = channels[4].Value * channels[8].Value ;	// CurrVLow * Vlow;
	snprintf(tableData[3].voltage, sizeof(tableData[3].voltage), "%.2f", channels[8].Value);
	snprintf(tableData[3].current, sizeof(tableData[3].current), "%.2f", channels[4].Value);
	snprintf(tableData[3].power, sizeof(tableData[3].power), "%.2f", PowerVlow);

	float  PowerVhigh = channels[5].Value * channels[9].Value ;	// CurrVHigh * Vhigh;
	snprintf(tableData[4].voltage, sizeof(tableData[4].voltage), "%.2f", channels[9].Value);
	snprintf(tableData[4].current, sizeof(tableData[4].current), "%.2f", channels[5].Value);
	snprintf(tableData[4].power, sizeof(tableData[4].power), "%.2f", PowerVhigh);


}


uint8_t read_register(uint8_t reg_addr)
{
	uint8_t rxBuffer[1];
	uint8_t reg[1];
	reg[0] = reg_addr;
	I2C_receive(STUSB4500_I2C_ADDR, reg, rxBuffer, 1, 1 ); // Stores received bytes in _rxBuffer
	return rxBuffer[0];
}



uint8_t write_register(uint8_t reg_addr , uint8_t reg_data)
{
	uint8_t reg[2];
	reg[0] = reg_addr;
	reg[1] = reg_data;
	I2C_transmit(STUSB4500_I2C_ADDR, reg,  2 );
}





uint8_t txBuffer[6];
uint32_t pdo_array[7];
uint32_t read_source_pdo(uint8_t pdo_number) {
    uint8_t base_addr = REG_RX_DATA_OBJ1_0; // İlk PDO'nun başlangıç adresi
    uint8_t reg_addr = base_addr + (pdo_number - 1) * 4; // PDO'nun başlangıç adresini hesapla

    uint32_t pdo = 0;
    // 4 byte PDO'yu oku
    pdo |= (read_register(reg_addr + 3) << 24); // MSB
    pdo |= (read_register(reg_addr + 2) << 16);
    pdo |= (read_register(reg_addr + 1) << 8);
    pdo |= read_register(reg_addr);            // LSB
    pdo_array[pdo_number - 1] = pdo;
    return pdo;
}

uint8_t PDOIndex =0;
uint8_t PDOtype =0;
uint32_t PDOvoltage =0;
uint32_t PDOcurrent =0;

uint32_t PDOmin_voltage =0;
uint32_t PDOmax_voltage =0;

void decode_source_pdo(uint32_t pdo) {
    char message[128]; // USB mesajı için buffer
    PDOtype = (pdo >> 30) & 0x3; // PDO Türü (Fixed, Battery, Variable)

    if (PDOtype == 0) { // Fixed PDO
        PDOvoltage = ((pdo >> 10) & 0x3FF) * 50; // Voltaj (50mV biriminde)
        PDOcurrent = (pdo & 0x3FF) * 10;         // Akım (10mA biriminde)
        sprintf(message, "Fixed PDO - Voltage: %u mV, Current: %u mA\r\n", PDOvoltage, PDOcurrent);
        USB_SendMessage(message);
    } else if (PDOtype == 1) { // Battery PDO
    	PDOvoltage = ((pdo >> 10) & 0x3FF) * 50; // Voltaj
    	PDOcurrent = (pdo & 0x3FF) * 250;          // Güç (250mW biriminde)
        sprintf(message, "Battery PDO - Voltage: %u mV, Power: %u mW\r\n", PDOvoltage, PDOcurrent);
        USB_SendMessage(message);
    } else if (PDOtype == 2) { // Variable PDO
        uint32_t min_voltage = ((pdo >> 20) & 0x3FF) * 50;
        uint32_t max_voltage = ((pdo >> 10) & 0x3FF) * 50;
        uint32_t current = (pdo & 0x3FF) * 10;
        sprintf(message, "Variable PDO - Min Voltage: %u mV, Max Voltage: %u mV, Current: %u mA\r\n",
                min_voltage, max_voltage, current);
        USB_SendMessage(message);
    } else {
        USB_SendMessage("Unknown PDO type.\r\n");
    }
}

void list_all_source_pdos(uint8_t total_pdos) {
	PDOIndex =1;
	char buffer[32];
    snprintf(buffer, sizeof(buffer), "PDO(%d) ",PDOtype );
    ST7789_WriteString(235, 70, buffer, Font_7x10, WHITE, BLACK);

    for (uint8_t i = 1; i <= total_pdos; i++) {
        uint32_t pdo = read_source_pdo(i);
        decode_source_pdo(pdo); // PDO çözümlemesi

        if (PDOtype == 2)  //Variable PDO
        {
            snprintf(buffer, sizeof(buffer), "Min:%dV Max:%dV @%dA ",PDOmin_voltage , PDOmax_voltage ,PDOcurrent );
            ST7789_WriteString(235, 70 + 12*i, buffer, Font_7x10, WHITE, BLACK);
        }
        else
        {
            snprintf(buffer, sizeof(buffer), "%dV @%dA ",PDOvoltage , PDOcurrent );
            ST7789_WriteString(235, 70 + 12*i, buffer, Font_7x10, WHITE, BLACK);
        }


    }
}
void read_sink_pdos() {
    uint8_t pdo[12]; // 3 PDO, her biri 4 byte
    char buffer[32];
    for (int i = 0; i < 12; i++) {
        pdo[i] = read_register(REG_DPM_SNK_PDO1_0 + i); // Register adreslerini sırasıyla oku
    }

    // PDO çözümle
    for (int i = 0; i < 3; i++) {
        uint32_t pdo_value = (pdo[i * 4 + 3] << 24) | (pdo[i * 4 + 2] << 16) | (pdo[i * 4 + 1] << 8) | pdo[i * 4];


        uint32_t voltage = ((pdo_value >> 10) & 0x3FF) * 50; // Voltaj, 50mV birimindedir
        uint32_t current = (pdo_value & 0x3FF) * 10;         // Akım, 10mA birimindedir
        printf("Voltage: %u mV, Current: %u mA\n", voltage/1000, current/1000);

        snprintf(buffer, sizeof(buffer), "%dV @%dA ",voltage/1000 , current/1000 );
        ST7789_WriteString(235, 70 + 12*i, buffer, Font_7x10, WHITE, BLACK);


    }
}


uint32_t create_fixed_pdo(uint32_t voltage_mv, uint32_t current_ma) {
    uint32_t voltage = (voltage_mv / 50) & 0x3FF; // 50mV biriminde voltaj
    uint32_t current = (current_ma / 10) & 0x3FF; // 10mA biriminde akım
    return (0 << 30) | (voltage << 10) | current; // Fixed PDO türü
}

void write_sink_pdos() {

	// PDO değerleri: Her biri 32 bitlik 3 örnek PDO
	uint32_t pdo_values[3] = {
		create_fixed_pdo(10000, 2000), // PDO1:
		create_fixed_pdo(10000, 3000), // PDO2:
		create_fixed_pdo(20000, 5000)  // PDO3:
	};

    // Her bir PDO değeri 4 byte'tır, bu nedenle num_pdos'a dikkat edin.
    for (int i = 0; i < 3; i++) {
        // PDO değerini 4 byte'a ayır ve registerlere sırayla yaz.
        write_register(REG_DPM_SNK_PDO1_0 + (i * 4), pdo_values[i] & 0xFF);          // LSB
        write_register(REG_DPM_SNK_PDO1_0 + (i * 4) + 1, (pdo_values[i] >> 8) & 0xFF);  // Byte 1
        write_register(REG_DPM_SNK_PDO1_0 + (i * 4) + 2, (pdo_values[i] >> 16) & 0xFF); // Byte 2
        write_register(REG_DPM_SNK_PDO1_0 + (i * 4) + 3, (pdo_values[i] >> 24) & 0xFF); // MSB
    }
}



void send_soft_reset() {
    write_register(0x51, 0x0D); // TX_HEADER_LOW = SOFT_RESET
    write_register(0x1A, 0x26); // PD_COMMAND_CTRL = SEND_COMMAND
}


void print_time_ms(uint8_t tag)
{
	uint32_t now = HAL_GetTick();
	char buffer[64];
	snprintf(buffer, sizeof(buffer), "Tag %u - Current Time: %lu ms\r\n", tag, now);
	USB_SendMessage(buffer);
}


uint32_t lastTimeA;
uint32_t lastTimeB;
uint32_t lastTimeC;
uint32_t lastTimeD;
uint32_t lastTimeE;

uint32_t currentTime1;
uint32_t currentTime2;
uint32_t currentTime3;
uint32_t currentTime4;

void SetPowerEnable(const char* name, bool enable) {
    GPIO_PinState state = enable ? GPIO_PIN_SET : GPIO_PIN_RESET;

    if (strcmp(name, "EN3V3") == 0) {
        HAL_GPIO_WritePin(EN1_GPIO_Port, EN1_Pin, state); // 3.3V
    } else if (strcmp(name, "EN2V5") == 0) {
        HAL_GPIO_WritePin(EN2_GPIO_Port, EN2_Pin, state); // 2.5V
    } else if (strcmp(name, "EN1V8") == 0) {
        HAL_GPIO_WritePin(EN3_GPIO_Port, EN3_Pin, state); // 1.8V
    } else if (strcmp(name, "ENLow") == 0) {
        HAL_GPIO_WritePin(EN4_GPIO_Port, EN4_Pin, state); // VAdjLow
    } else if (strcmp(name, "ENHigh") == 0) {
        HAL_GPIO_WritePin(EN5_GPIO_Port, EN5_Pin, state); // VAdjHigh
    }
}

// Draw all arrows and circles (init screen)
void DrawMenuArrowsAndCirclesInit(void)
{
    // 1.8V
    DrawChannelCircle(0, channelEnabled[0]);
    DrawChannelCircleFrame(0, WHITE);
    // 2.5V
    DrawChannelCircle(1, channelEnabled[1]);
    DrawChannelCircleFrame(1, WHITE);
    // 3.3V
    DrawChannelCircle(2, channelEnabled[2]);
    DrawChannelCircleFrame(2, WHITE);

    // VLow Channel (Row 3)
    DrawChannelCircle(3, channelEnabled[3]);
    DrawChannelArrows(3);
    DrawChannelCircleFrame(3, WHITE);
    DrawChannelArrowFrame(3, WHITE);

    // VHigh Channel (Row 4)
    DrawChannelCircle(4, channelEnabled[4]);
    DrawChannelArrows(4);
    DrawChannelCircleFrame(4, WHITE);
    DrawChannelArrowFrame(4, WHITE);
}

void DrawChannelCircle(uint8_t row, bool enabled)
{
    int y_position = 30 + row * 25;
    int xOffset = 10;
    int circleCenterX = 240 + xOffset;  // SAME for all rows
    int centerY = y_position + 8;
    int circleRadius = 6;
    ST7789_DrawFilledCircle(circleCenterX, centerY, circleRadius, enabled ? GREEN : RED);
}

void DrawChannelArrows(uint8_t row)
{
    int y_position = 30 + row * 25;
    int arrowOffset = 10 + 14;
    int centerY = y_position + 8;
    // Up Arrow
    ST7789_DrawFilledTriangle(251 + arrowOffset, centerY - 7,
                              246 + arrowOffset, centerY - 2,
                              256 + arrowOffset, centerY - 2, WHITE);
    // Down Arrow
    ST7789_DrawFilledTriangle(251 + arrowOffset, centerY + 7,
                              246 + arrowOffset, centerY + 2,
                              256 + arrowOffset, centerY + 2, WHITE);
}


void DrawChannelCircleFrame(uint8_t row, uint16_t color)
{
    int y_position = 30 + row * 25;
    int xOffset = 10;
    int circleCenterX = 240 + xOffset;
    int circleRadius = 6;

    // Draw thin blue frame (single line)
    ST7789_DrawRectangle(circleCenterX - circleRadius - 2, y_position - 2,
                         circleCenterX + circleRadius + 2, y_position + 18, color);
}


void DrawChannelArrowFrame(uint8_t row, uint16_t color)
{
    int y_position = 30 + row * 25;
    int xOffset = 10;
    int arrowOffset = xOffset + 14;

    // Single thin blue frame
    ST7789_DrawRectangle(242 + arrowOffset, y_position - 2,
                         260 + arrowOffset, y_position + 18, color);
}



void UpdateChannelCircle(uint8_t row, bool enabled)
{
    // Call this after menu action for row 3 (VLow) or row 4 (VHigh)
    DrawChannelCircle(row, enabled);
}

void UpdateChannelFrame(uint8_t row, bool OnlyCircleFrame)
{
    DrawChannelFrame(row,OnlyCircleFrame);
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
  MX_ADC_Init();
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  SetPowerEnable("EN3V3", false);
  SetPowerEnable("EN2V5", false);
  SetPowerEnable("EN1V8", false);
  SetPowerEnable("ENLow", false);
  SetPowerEnable("ENHigh", false);


  // ADC Kalibrasyon
  if (HAL_ADCEx_Calibration_Start(&hadc) != HAL_OK) {
      USB_SendMessage("ADC calibration failed\r\n");
      Error_Handler();
  }



  USB_SendMessage(welcomeMessage);


  ST7789_Init(); // Ekranı başlat
  USB_SendMessage("Display Initialized.\r\n");
  ST7789_Fill_Color(BLACK); // Ekranı beyaz ile doldur
  //ST7789_DrawRectangle(0, 0, ST7789_WIDTH-1, ST7789_HEIGHT-1, YELLOW);

  DisplayTableWithGrid();
  HAL_Delay(100);

  I2C_init();
  I2C2_init();
  I2C3_init();
  //SoftI2C_init(&i2c2);
  //SoftI2C_init(&i2c3);
  SetVadjLVoltage(5.00);
  SetVadjHVoltage(12.00);


  HAL_Delay(1);

   write_sink_pdos();// Once okuyup farklı ise yazma yapılırsa flash omrunden yemez.
   send_soft_reset();

   HAL_Delay(10);
   //write_register(REG_RESET_CTRL,1);
   //HAL_Delay(25);
   //write_register(REG_RESET_CTRL,0);
   //HAL_Delay(100);

   write_register(0x51, 0x03);
   //write_register(REG_DPM_PDO_NUMB,1);
   HAL_Delay(10);

   uint32_t count =  read_register(REG_DPM_PDO_NUMB);
   read_active_pdo();
   //list_all_source_pdos();
   //MCP4725_write_dac(&i2c2, 2048);
   //MCP4725_write_dac(&i2c3, 1024);

    bool SkpFirstRunCheck = false;

   DrawMenuArrowsAndCirclesInit();

   HAL_Delay(200);

   SetPowerEnable("EN3V3", true);
   SetPowerEnable("EN2V5", true);
   SetPowerEnable("EN1V8", true);
   SetPowerEnable("ENLow", true);
   SetPowerEnable("ENHigh", true);

   HAL_Delay(800); // Wait Untill current Draw for on board caps reach steady state.

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  uint32_t currentTime = micros();
      HAL_GPIO_TogglePin(GPIOB, LED_BLUE_Pin);
      CheckRotaryButtonPressOnly();


      if (rotaryPressed) {
    	  rotaryPressed = false;
          RedrawAllTableRows();  // Artık burda çizim olur, interrupt hızlı kalır
      }

       GetADCValuesWithStatsFast();
       CheckAndDisableOvercurrent();


      if (menuState == MENU_LIMIT_SET_ACTIVE || menuState == MENU_LIMIT_SET)
      {
          if ((currentTime - lastTimeD) > 100000) {
        	  WriteExtraInfoStepByStep();
        	  lastTimeD = currentTime;
          }
    	  continue;
      }



  ////////////// End of I2C test
      static uint16_t ramp = 0;
      ramp += 32;
      set_dacVLow(ramp);

      static bool toggle = false;

      if ((currentTime - lastTimeC) >= 1000) {
          lastTimeC = currentTime;


          //SetVadjLVoltage(dacValueVlow);
          //SetVadjHVoltage(dacValueVhigh  );
          toggle = !toggle;  // Toggle değiştir
      }


      	 // call every 500ms
       if ((currentTime - lastTimeA) >= 500000) {
           lastTimeA = currentTime;   //

       }

       if ((currentTime - lastTimeB) > 30000) {
    	   StepwiseUpdateDisplay(); // burda herbir case in zman tüketimini bastır ve zmnlmlrı dağıt .
           lastTimeB = currentTime;
       }



       uint32_t elapsed = micros() - currentTime;
       if (elapsed >= 10000 && SkpFirstRunCheck)
          {
              HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
          }

       SkpFirstRunCheck = true;


    //  HAL_Delay(1500);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI14|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
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
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = ENABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_2;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_3;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_4;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_5;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_6;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_7;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_8;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_9;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_10;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_11;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_12;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_13;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_14;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_15;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

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
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 38400;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_MultiProcessor_Init(&huart3, 0, UART_WAKEUPMETHOD_IDLELINE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, EN1_Pin|EN4_Pin|EN5_Pin|LED_RED_Pin
                          |LED_BLUE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, SCL3_Pin|SDA3_Pin|SCL2_Pin|SDA2_Pin
                          |TFT_DC_Pin|TFT_RES_Pin|EN3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SCL1_Pin|ALERT__Pin|EN2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SDA1_GPIO_Port, SDA1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : KEY_Pin S2_Pin */
  GPIO_InitStruct.Pin = KEY_Pin|S2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : EN1_Pin EN4_Pin EN5_Pin LED_RED_Pin
                           LED_BLUE_Pin */
  GPIO_InitStruct.Pin = EN1_Pin|EN4_Pin|EN5_Pin|LED_RED_Pin
                          |LED_BLUE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : SCL3_Pin SCL2_Pin TFT_DC_Pin TFT_RES_Pin
                           EN3_Pin */
  GPIO_InitStruct.Pin = SCL3_Pin|SCL2_Pin|TFT_DC_Pin|TFT_RES_Pin
                          |EN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SDA3_Pin SDA2_Pin */
  GPIO_InitStruct.Pin = SDA3_Pin|SDA2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : SCL1_Pin ALERT__Pin EN2_Pin */
  GPIO_InitStruct.Pin = SCL1_Pin|ALERT__Pin|EN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SDA1_Pin */
  GPIO_InitStruct.Pin = SDA1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SDA1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI1_CS_Pin */
  GPIO_InitStruct.Pin = SPI1_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI1_CS_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

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
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
