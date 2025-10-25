#include "main.h"
#include "stm32_sw_i2c.h"
#include "stusb4500.h"

//void I2C_bus_init(uint8_t scl_pin, uint8_t sda_pin, uint8_t port){
//	  /*Configure GPIO pins : SW_I2C_SCL_Pin SW_I2C_SDA_Pin */
//	  GPIO_InitStruct.Pin = SW_I2C_SCL_Pin|SW_I2C_SDA_Pin;
//	  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
//	  GPIO_InitStruct.Pull = GPIO_PULLUP;
//	  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
//	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
//}

#define true 1
#define false 0

#define I2C1_GPIO_Port GPIOA
#define I2C1_CLEAR_SDA HAL_GPIO_WritePin(I2C1_GPIO_Port, SDA1_Pin, GPIO_PIN_RESET)
#define I2C1_SET_SDA HAL_GPIO_WritePin(I2C1_GPIO_Port, SDA1_Pin, GPIO_PIN_SET)
#define I2C1_CLEAR_SCL HAL_GPIO_WritePin(I2C1_GPIO_Port, SCL1_Pin, GPIO_PIN_RESET)
#define I2C1_SET_SCL HAL_GPIO_WritePin(I2C1_GPIO_Port, SCL1_Pin, GPIO_PIN_SET)
#define I2C_DELAY delay_us(1)


void delay_us(uint32_t us) {
    uint32_t startTick = SysTick->VAL;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((startTick - SysTick->VAL) < ticks);
}

void I2C_init(void)
{
    I2C1_SET_SDA;
    I2C1_SET_SCL;
}

void I2C_start_cond(void)
{
    I2C1_SET_SCL;
    I2C1_SET_SDA;
    I2C_DELAY;
    I2C1_CLEAR_SDA;
    I2C_DELAY;
    I2C1_CLEAR_SCL;
    I2C_DELAY;
}

void I2C_stop_cond(void)
{
    I2C1_CLEAR_SDA;
    I2C_DELAY;
    I2C1_SET_SCL;
    I2C_DELAY;
    I2C1_SET_SDA;
    I2C_DELAY;
}

void I2C_write_bit(uint8_t b)
{
    if (b > 0)
        I2C1_SET_SDA;
    else
        I2C1_CLEAR_SDA;

    I2C_DELAY;
    I2C1_SET_SCL;
    I2C_DELAY;
    I2C1_CLEAR_SCL;
}

uint8_t I2C_read_SDA(void)
{
    if (HAL_GPIO_ReadPin(I2C1_GPIO_Port, SDA1_Pin) == GPIO_PIN_SET)
        return 1;
    else
        return 0;
    return 0;
}

// Reading a bit in I2C:
uint8_t I2C_read_bit(void)
{
    uint8_t b;

    I2C1_SET_SDA;
    I2C_DELAY;
    I2C1_SET_SCL;
    I2C_DELAY;

    b = I2C_read_SDA();

    I2C1_CLEAR_SCL;

    return b;
}

_Bool I2C_write_byte(uint8_t B,
                     _Bool start,
                     _Bool stop)
{
    uint8_t ack = 0;

    if (start)
        I2C_start_cond();

    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        I2C_write_bit(B & 0x80); // write the most-significant bit
        B <<= 1;
    }

    ack = I2C_read_bit();

    if (stop)
        I2C_stop_cond();

    return !ack; //0-ack, 1-nack
}

// Reading a byte with I2C:
uint8_t I2C_read_byte(_Bool ack, _Bool stop)
{
    uint8_t B = 0;

    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        B <<= 1;
        B |= I2C_read_bit();
    }

    if (ack)
        I2C_write_bit(0);
    else
        I2C_write_bit(1);

    if (stop)
        I2C_stop_cond();

    return B;
}

// Sending a byte with I2C:
_Bool I2C_send_byte(uint8_t address,
                    uint8_t data)
{
    //    if( I2C_write_byte( address << 1, true, false ) )   // start, send address, write
    if (I2C_write_byte(address, true, false)) // start, send address, write
    {
        // send data, stop
        if (I2C_write_byte(data, false, true))
            return true;
    }

    I2C_stop_cond(); // make sure to impose a stop if NAK'd
    return false;
}

// Receiving a byte with a I2C:
uint8_t I2C_receive_byte(uint8_t address)
{
    if (I2C_write_byte((address << 1) | 0x01, true, false)) // start, send address, read
    {
        return I2C_read_byte(false, true);
    }

    return 0; // return zero if NAK'd
}

// Sending a byte of data with I2C:
_Bool I2C_send_byte_data(uint8_t address,
                         uint8_t reg,
                         uint8_t data)
{
    //    if( I2C_write_byte( address << 1, true, false ) )   // start, send address, write
    if (I2C_write_byte(address, true, false))
    {
        if (I2C_write_byte(reg, false, false)) // send desired register
        {
            if (I2C_write_byte(data, false, true))
                return true; // send data, stop
        }
    }

    I2C_stop_cond();
    return false;
}

// Receiving a byte of data with I2C:
uint8_t I2C_receive_byte_data(uint8_t address,
                              uint8_t reg)
{
    //if( I2C_write_byte( address << 1, true, false ) )   // start, send address, write
    if (I2C_write_byte(address, true, false))
    {
        if (I2C_write_byte(reg, false, false)) // send desired register
        {
            if (I2C_write_byte((address << 1) | 0x01, true, false)) // start again, send address, read
            {
                return I2C_read_byte(false, true); // read data
            }
        }
    }

    I2C_stop_cond();
    return 0; // return zero if NACKed
}


//HAL_StatusTypeDef I2C_Read_USB_PD(uint8_t Port, uint16_t Address ,uint8_t *DataR ,uint16_t Length)
//{
//  return  HAL_I2C_Mem_Read(hi2c[Port],(I2cDeviceID_7bit<<1), Address ,AddressSize, DataR, Length ,2000);
//}

//HAL_StatusTypeDef I2C_Write_USB_PD(uint8_t Port, uint16_t Address ,uint8_t *DataW ,uint16_t Length)
//{
//  return  HAL_I2C_Mem_Write(hi2c[Port],(I2cDeviceID_7bit<<1), Address ,AddressSize, DataW, Length, 2000 ); // unmask all alarm status
//}



HAL_StatusTypeDef I2C_Write_USB_PD(uint8_t Port, uint16_t Address, uint8_t *DataW, uint16_t Length)
{
		uint8_t  Buffer[2];
	    Buffer[0]= (uint8_t)Address ;
	    Buffer[1] = DataW[0];
	     if (I2C_transmit(STUSB4500_I2C_ADDR,Buffer,2))
	    	 return HAL_OK;
	     else
	    	 return HAL_ERROR;
}



HAL_StatusTypeDef I2C_Read_USB_PD(uint8_t Port, uint16_t Address ,uint8_t *DataR ,uint16_t Length)
{
	uint8_t rxBuffer[1];
	uint8_t reg_addr[1];
	reg_addr[0] = (uint8_t)Address;

	if (I2C_receive(STUSB4500_I2C_ADDR, reg_addr,rxBuffer,1,1))
		return HAL_OK;
	else
		return HAL_ERROR;
}


_Bool I2C_transmit(uint8_t address, uint8_t data[], uint8_t size)
{
    if (I2C_write_byte(address, true, false)) // first byte
    {
        for (int i = 0; i < size; i++)
        {
            if (i == size - 1)
            {
                if (I2C_write_byte(data[i], false, true))
                    return true;
            }
            else
            {
                if (!I2C_write_byte(data[i], false, false))
                    break; //last byte
            }
        }
    }

    I2C_stop_cond();
    return false;
}

_Bool I2C_receive(uint8_t address, uint8_t reg[], uint8_t *data, uint8_t reg_size, uint8_t size)
{
    if (I2C_write_byte(address, true, false))
    {
        for (int i = 0; i < reg_size; i++)
        {
            if (!I2C_write_byte(reg[i], false, false))
                break;
        }
        if (I2C_write_byte(address | 0x01, true, false)) // start again, send address, read (LSB signifies R or W)
        {
            for (int j = 0; j < size; j++)
            {
                *data++ = I2C_read_byte(false, false); // read data
            }
            I2C_stop_cond();
            return true;
        }
    }
    I2C_stop_cond();
    return false;
}
