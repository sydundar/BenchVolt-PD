/*
  stm32_sw_I2C.h - Library for implementing I2C3 in SW on any GPIO on STM32 MCU.
  Created by Mawaba Pascal Dao, Dec 2, 2020.
  Released into the public domain.
*/


#include "main.h"


_Bool set_dacVLow(uint16_t reg_data);
void I2C3_init(void);
void I2C3_start_cond(void);
void I2C3_stop_cond(void);
void I2C3_write_bit(uint8_t b);
uint8_t I2C3_read_SDA(void);
uint8_t I2C3_read_bit(void);
_Bool I2C3_write_byte(uint8_t B, _Bool start, _Bool stop);
uint8_t I2C3_read_byte(_Bool ack, _Bool stop);
_Bool I2C3_send_byte(uint8_t address, uint8_t data);
uint8_t I2C3_receive_byte(uint8_t address);
_Bool I2C3_send_byte_data(uint8_t address, uint8_t reg, uint8_t data);
uint8_t I2C3_receive_byte_data(uint8_t address, uint8_t reg);
_Bool I2C3_transmit(uint8_t address, uint8_t data[], uint8_t size);
_Bool I2C3_receive(uint8_t address, uint8_t reg[], uint8_t *data, uint8_t reg_size, uint8_t size);

HAL_StatusTypeDef I2C3_Write_USB_PD(uint8_t Port, uint16_t Address, uint8_t *DataW, uint16_t Length);

HAL_StatusTypeDef I2C3_Read_USB_PD(uint8_t Port, uint16_t Address ,uint8_t *DataR ,uint16_t Length);


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////


/* -------------------------------------------------------------------------- */
/* TPS55289 REGISTER MAP & CONSTANTS                                          */
/* -------------------------------------------------------------------------- */


#define REG_REF_LSB         0x00  // Reference Voltage LSB
#define REG_REF_MSB         0x01  // Reference Voltage MSB
#define REG_IOUT_LIMIT      0x02  // Current Limit Setting
#define REG_VOUT_SR         0x03  // Slew Rate
#define REG_VOUT_FS         0x04  // Feedback Selection
#define REG_CDC             0x05  // Cable Compensation
#define REG_MODE            0x06  // Mode Control (OE bit burada)
#define REG_STATUS          0x07  // Status

#define TPS_VREF_OFFSET_V   0.045f
#define TPS_VREF_LSB_V      0.0005645f
#define TPS_VREF_MAXCODE    0x7FE
#define TPS_FB_RATIO_VAL    0.0564f
#define TPS_VOUT_MIN_V      0.8f
#define TPS_VOUT_MAX_V      22.0f
#define TPS_ISENSE_LSB_V    0.0005f
#define TPS_RSENSE_OHM      0.010f  // 10 mOhm

/* -------------------------------------------------------------------------- */
/* TPS55289 FUNCTION PROTOTYPES                                               */
/* -------------------------------------------------------------------------- */
void InitVAdjHigh();
void EnVAdjHighOutput(bool onoff);
void SetVoltageVAdjHigh(float voltage);
void SetVAdjHighCurrentLimit(float current);
uint8_t VAdjHigh_GetStatus();

