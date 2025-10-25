#include <soft_i2c_2.h>

#define true 1
#define false 0

#define I2C2_GPIO_Port GPIOC
#define I2C2_CLEAR_SDA HAL_GPIO_WritePin(I2C2_GPIO_Port, SDA2_Pin, GPIO_PIN_RESET)
#define I2C2_SET_SDA HAL_GPIO_WritePin(I2C2_GPIO_Port, SDA2_Pin, GPIO_PIN_SET)
#define I2C2_CLEAR_SCL HAL_GPIO_WritePin(I2C2_GPIO_Port, SCL2_Pin, GPIO_PIN_RESET)
#define I2C2_SET_SCL HAL_GPIO_WritePin(I2C2_GPIO_Port, SCL2_Pin, GPIO_PIN_SET)
#define I2C2_DELAY delay2_us(1)

#define TMP1075_ADDR 0x48<<1
#define MCP4725_ADDR 0x64<<1


float read_temp(uint8_t reg_data)
{
	uint8_t rxBuffer[2];
	uint8_t reg[1];
	reg[0] = reg_data;
	I2C2_receive(TMP1075_ADDR, reg, rxBuffer, 1, 2 );
	int16_t  raw_temp = (rxBuffer[0] << 8) | rxBuffer[1];

	raw_temp >>= 4;
	float temp_c = raw_temp * 0.0625f;
	return temp_c;
}

_Bool set_dacVLow(uint16_t reg_data)
{
    reg_data &= 0x0FFF;  // 12-bit sınırla
    uint8_t data[2];
    data[0] = (reg_data >> 8) & 0x0F;    // D11..D8
    data[0] |= 0x00;                     // C2:C1:PD1:PD0 = 0b0000

    data[1] = reg_data & 0xFF;           // D7..D0

     return I2C2_transmit(MCP4725_ADDR, data, 2);
}




void delay2_us(uint32_t us) {
    uint32_t startTick = SysTick->VAL;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((startTick - SysTick->VAL) < ticks);
}

void I2C2_init(void)
{
    I2C2_SET_SDA;
    I2C2_SET_SCL;
}

void I2C2_start_cond(void)
{
    I2C2_SET_SCL;
    I2C2_SET_SDA;
    I2C2_DELAY;
    I2C2_CLEAR_SDA;
    I2C2_DELAY;
    I2C2_CLEAR_SCL;
    I2C2_DELAY;
}

void I2C2_stop_cond(void)
{
    I2C2_CLEAR_SDA;
    I2C2_DELAY;
    I2C2_SET_SCL;
    I2C2_DELAY;
    I2C2_SET_SDA;
    I2C2_DELAY;
}

void I2C2_write_bit(uint8_t b)
{
    if (b > 0)
        I2C2_SET_SDA;
    else
        I2C2_CLEAR_SDA;

    I2C2_DELAY;
    I2C2_SET_SCL;
    I2C2_DELAY;
    I2C2_CLEAR_SCL;
}

uint8_t I2C2_read_SDA(void)
{
    if (HAL_GPIO_ReadPin(I2C2_GPIO_Port, SDA2_Pin) == GPIO_PIN_SET)
        return 1;
    else
        return 0;
    return 0;
}

// Reading a bit in I2C2:
uint8_t I2C2_read_bit(void)
{
    uint8_t b;

    I2C2_SET_SDA;
    I2C2_DELAY;
    I2C2_SET_SCL;
    I2C2_DELAY;

    b = I2C2_read_SDA();

    I2C2_CLEAR_SCL;

    return b;
}

_Bool I2C2_write_byte(uint8_t B,
                     _Bool start,
                     _Bool stop)
{
    uint8_t ack = 0;

    if (start)
        I2C2_start_cond();

    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        I2C2_write_bit(B & 0x80); // write the most-significant bit
        B <<= 1;
    }

    ack = I2C2_read_bit();

    if (stop)
        I2C2_stop_cond();

    return !ack; //0-ack, 1-nack
}

// Reading a byte with I2C2:
uint8_t I2C2_read_byte(_Bool ack, _Bool stop)
{
    uint8_t B = 0;

    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        B <<= 1;
        B |= I2C2_read_bit();
    }

    if (ack)
        I2C2_write_bit(0);
    else
        I2C2_write_bit(1);

    if (stop)
        I2C2_stop_cond();

    return B;
}

// Sending a byte with I2C2:
_Bool I2C2_send_byte(uint8_t address,
                    uint8_t data)
{
    //    if( I2C2_write_byte( address << 1, true, false ) )   // start, send address, write
    if (I2C2_write_byte(address, true, false)) // start, send address, write
    {
        // send data, stop
        if (I2C2_write_byte(data, false, true))
            return true;
    }

    I2C2_stop_cond(); // make sure to impose a stop if NAK'd
    return false;
}

// Receiving a byte with a I2C2:
uint8_t I2C2_receive_byte(uint8_t address)
{
    if (I2C2_write_byte((address << 1) | 0x01, true, false)) // start, send address, read
    {
        return I2C2_read_byte(false, true);
    }

    return 0; // return zero if NAK'd
}

// Sending a byte of data with I2C2:
_Bool I2C2_send_byte_data(uint8_t address,
                         uint8_t reg,
                         uint8_t data)
{
    //    if( I2C2_write_byte( address << 1, true, false ) )   // start, send address, write
    if (I2C2_write_byte(address, true, false))
    {
        if (I2C2_write_byte(reg, false, false)) // send desired register
        {
            if (I2C2_write_byte(data, false, true))
                return true; // send data, stop
        }
    }

    I2C2_stop_cond();
    return false;
}

// Receiving a byte of data with I2C2:
uint8_t I2C2_receive_byte_data(uint8_t address,
                              uint8_t reg)
{
    //if( I2C2_write_byte( address << 1, true, false ) )   // start, send address, write
    if (I2C2_write_byte(address, true, false))
    {
        if (I2C2_write_byte(reg, false, false)) // send desired register
        {
            if (I2C2_write_byte((address << 1) | 0x01, true, false)) // start again, send address, read
            {
                return I2C2_read_byte(false, true); // read data
            }
        }
    }

    I2C2_stop_cond();
    return 0; // return zero if NACKed
}




_Bool I2C2_transmit(uint8_t address, uint8_t data[], uint8_t size)
{
    if (I2C2_write_byte(address, true, false)) // first byte
    {
        for (int i = 0; i < size; i++)
        {
            if (i == size - 1)
            {
                if (I2C2_write_byte(data[i], false, true))
                    return true;
            }
            else
            {
                if (!I2C2_write_byte(data[i], false, false))
                    break; //last byte
            }
        }
    }

    I2C2_stop_cond();
    return false;
}

_Bool I2C2_receive(uint8_t address, uint8_t reg[], uint8_t *data, uint8_t reg_size, uint8_t size)
{
    if (I2C2_write_byte(address, true, false))
    {
        for (int i = 0; i < reg_size; i++)
        {
            if (!I2C2_write_byte(reg[i], false, false))
                break;
        }
        if (I2C2_write_byte(address | 0x01, true, false)) // start again, send address, read
        {
            for (int j = 0; j < size; j++)
            {
                _Bool ack = (j < size - 1);
                _Bool stop = (j == size - 1);
                *data++ = I2C2_read_byte(ack, stop);
            }
            return true;
        }
    }
    I2C2_stop_cond();
    return false;
}

