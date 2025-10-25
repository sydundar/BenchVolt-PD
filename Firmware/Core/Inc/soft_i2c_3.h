/*
  stm32_sw_I2C.h - Library for implementing I2C3 in SW on any GPIO on STM32 MCU.
  Created by Mawaba Pascal Dao, Dec 2, 2020.
  Released into the public domain.
*/


#include "main.h"


_Bool set_dacVHigh(uint16_t reg_data);

void I2C3_init(void);

void I2C3_start_cond(void);

void I2C3_stop_cond(void);

void I2C3_write_bit(uint8_t b);

uint8_t I2C3_read_SDA(void);

// Reading a bit in I2C3:
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


