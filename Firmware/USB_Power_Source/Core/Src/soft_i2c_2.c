#include <soft_i2c_2.h>

#define true 1
#define false 0

#define I2C2_GPIO_Port GPIOC
#define I2C2_CLEAR_SDA HAL_GPIO_WritePin(I2C2_GPIO_Port, SDA2_Pin, GPIO_PIN_RESET)
#define I2C2_SET_SDA HAL_GPIO_WritePin(I2C2_GPIO_Port, SDA2_Pin, GPIO_PIN_SET)
#define I2C2_CLEAR_SCL HAL_GPIO_WritePin(I2C2_GPIO_Port, SCL2_Pin, GPIO_PIN_RESET)
#define I2C2_SET_SCL HAL_GPIO_WritePin(I2C2_GPIO_Port, SCL2_Pin, GPIO_PIN_SET)
#define I2C2_DELAY delay2_us(1)

#define DC1_ADDRS	0x75<<1
#define DC2_ADDRS	0x74<<1
#define TMP1075_ADDR 0x48<<1


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



// Sending a byte of data with I2C2:
_Bool I2C2_send_byte_data(uint8_t address,
                         uint8_t reg,
                         uint8_t data)
{
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

    if (I2C2_write_byte(address, true, false))
    {
        if (I2C2_write_byte(reg, false, false)) // send desired register
        {
            if (I2C2_write_byte((address) | 0x01, true, false)) // start again, send address, read
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void TPS55289_InitInternalFeedback(uint8_t address) {
	uint8_t status = I2C2_receive_byte_data(address, REG_STATUS);
	uint8_t vfs = I2C2_receive_byte_data(address, REG_VOUT_FS);
    if (vfs == 0xFF) return;
    vfs &= ~(0x80 | 0x03);
    vfs |= 0x03;
    I2C2_send_byte_data(address, REG_VOUT_FS, vfs);
}

void TPS55289_EnableOutput(uint8_t address,_Bool en) {
    uint8_t mode = I2C2_receive_byte_data(address, REG_MODE);
    if (en) mode |= 0x80;
    else mode &= ~0x80;
    I2C2_send_byte_data(address, REG_MODE, mode);
}

void TPS55289_SetVoltage(uint8_t address, float voltage) {
    if (voltage < TPS_VOUT_MIN_V) voltage = TPS_VOUT_MIN_V;
    if (voltage > TPS_VOUT_MAX_V) voltage = TPS_VOUT_MAX_V;

    float vrefV = voltage * TPS_FB_RATIO_VAL;
    float codeF = (vrefV - TPS_VREF_OFFSET_V) / TPS_VREF_LSB_V;

    if (codeF < 0.0f) codeF = 0.0f;
    if (codeF > TPS_VREF_MAXCODE) codeF = (float)TPS_VREF_MAXCODE;

    uint16_t code = (uint16_t)(codeF + 0.5f);
    I2C2_send_byte_data(address, REG_REF_LSB, (uint8_t)(code & 0xFF));
    I2C2_send_byte_data(address, REG_REF_MSB, (uint8_t)((code >> 8) & 0x07));
}

void TPS55289_SetCurrentLimit(uint8_t address, float current) {
    if (current <= 0.0f) {
        I2C2_send_byte_data(address, REG_IOUT_LIMIT, 0x00);
        return;
    }
    float vsenseV = current * TPS_RSENSE_OHM;
    float codeF = vsenseV / TPS_ISENSE_LSB_V;
    if (codeF > 127.0f) codeF = 127.0f;

    uint8_t code = (uint8_t)(codeF + 0.5f);
    I2C2_send_byte_data(address, REG_IOUT_LIMIT, (0x80 | (code & 0x7F)));
}

uint8_t TPS55289_GetStatus(uint8_t address) {
    return I2C2_receive_byte_data(address, REG_STATUS);
}


//////////////////////////////////////////
void InitDC1() { TPS55289_InitInternalFeedback(DC1_ADDRS); }
void InitDC2() { TPS55289_InitInternalFeedback(DC2_ADDRS); }
void EnDC1Output(bool onoff) { TPS55289_EnableOutput(DC1_ADDRS,onoff); }
void EnDC2Output(bool onoff) { TPS55289_EnableOutput(DC2_ADDRS,onoff); }
void SetVoltageDC1(float voltage){ TPS55289_SetVoltage(DC1_ADDRS,voltage); }
void SetVoltageDC2(float voltage){ TPS55289_SetVoltage(DC2_ADDRS,voltage); }
void SetDC1CurrentLimit(float current) { TPS55289_SetCurrentLimit(DC1_ADDRS,current); }
void SetDC2CurrentLimit(float current) { TPS55289_SetCurrentLimit(DC2_ADDRS,current); }
uint8_t ReadStatusDC1()  { TPS55289_GetStatus(DC1_ADDRS); }
uint8_t ReadStatusDC2()  { TPS55289_GetStatus(DC2_ADDRS); }
