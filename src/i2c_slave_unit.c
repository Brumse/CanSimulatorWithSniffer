#include "i2c_slave_unit.h"

//slaveAddress slave;                 // bara liten baseline 
enum i2c_err_t {
    I2C_SUCCESS = 0,
    I2C_ERR_CRC,
    I2C_ERR_LEN,
    I2C_ERR_I2C,
    I2C_NUM_OF_ERR
};

#mcu_interrupt_on_i2c
i2c_err_t i2c_slave_unit_ISR(void) {
    
int8_t byte;
int8_t sendByte;
static int8_t cmd;
static int8_t rxCnt;
static int8_t txCnt;

switch (state)
{
    case I2C_IDLE:

        if (!BUF_EMPTY) 
            break;

        rxCnt = 0;
        txCnt = 0;    
        state = state.I2C_EVENT;
            break;

        case I2C_EVENT:

        byte = I2CBUF; 
        buf[rxCnt++] = byte;

        if (rxCnt != TOTALT NUMMER AV LÄNGD AV DATA VI ÄR UTE EFTER)
            break;

        handleData = true;  // lämna ISR, gå o gör skit i mainloopen

    case I2C_WRITE_DATA

        if (txCnt > TOTALT NUMMER AV LÄNGD AV DATA VI ÄR UTE EFTER) 
        {
            I2CTXBUF = SENDBUF[txCnt++];
        }
        else
        {
            I2CTXBUF = crc;        // skicka crc som sista
            state = I2C_IDLE;
        }

        break;
}
void i2c_slave_unit_init(void) {

}

