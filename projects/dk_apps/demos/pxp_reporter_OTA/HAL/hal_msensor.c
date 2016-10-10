/*
 * hal_msensor.c
 *
 *  Created on: 2016Äê8ÔÂ5ÈÕ
 *      Author: Administrator
 */

#include "hal_msensor.h"
#include "ad_i2c.h"
#include "osal.h"
#include "platform_devices.h"
#include "..\LIB\easylogger\elog.h"

static char tag[]="hal_msensor";
#define log_a(...) elog_a(tag, __VA_ARGS__)
#define log_e(...) elog_e(tag, __VA_ARGS__)
#define log_w(...) elog_w(tag, __VA_ARGS__)
#define log_i(...) elog_i(tag, __VA_ARGS__)
#define log_d(...) elog_d(tag, __VA_ARGS__)
#define log_v(...) elog_v(tag, __VA_ARGS__)

i2c_device FXOS8700_dev;
i2c_device FXOS21002_dev;


int8 hal_msensor_clear_motion_int(void)
{
	int8 status = 0;		// I2C transaction status

	uint8 iWhoAmI;		// sensor WhoAmI

	// check the WHOAMI register for the correct value and return immediately if invalid
	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_WHO_AM_I, &iWhoAmI, 1);
	status &= (iWhoAmI == FXOS8700_WHO_AM_I_VALUE) || (iWhoAmI == FXOS8700_WHO_AM_I_VALUE_ENG) || (iWhoAmI == FXOS8701_WHO_AM_I_VALUE);
	if (!status)
	{
		return HAL_MSENSOR_ERROR;
	}
	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_A_FFMT_SRC, &iWhoAmI, 1);

	return status;
}
int8 hal_msensor_clear_pluse_int(void)
{
	int8 status = 0;		// I2C transaction status

	uint8 iWhoAmI;		// sensor WhoAmI

	// check the WHOAMI register for the correct value and return immediately if invalid
	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_WHO_AM_I, &iWhoAmI, 1);
	status &= (iWhoAmI == FXOS8700_WHO_AM_I_VALUE) || (iWhoAmI == FXOS8700_WHO_AM_I_VALUE_ENG) || (iWhoAmI == FXOS8701_WHO_AM_I_VALUE);
	if (!status)
	{
		return HAL_MSENSOR_ERROR;
	}
	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_PLUSE_SRC, &iWhoAmI, 1);

	return status;
}

int8 hal_msensor_set_detect_mode(void)
{
	int8 status = 0;		// I2C transaction status

	uint8 iWhoAmI;		// sensor WhoAmI

	// check the WHOAMI register for the correct value and return immediately if invalid
	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_WHO_AM_I, &iWhoAmI, 1);
	status &= (iWhoAmI == FXOS8700_WHO_AM_I_VALUE) || (iWhoAmI == FXOS8700_WHO_AM_I_VALUE_ENG) || (iWhoAmI == FXOS8701_WHO_AM_I_VALUE);
	if (!status)
	{
		return HAL_MSENSOR_ERROR;
	}

	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG1, 0x00);


//	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_A_FFMT_CFG, 0xf8);
//	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_A_FFMT_THS, 0x30);		//  3g		63mg/LSB
//	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_A_FFMT_COUNT, 0x05);		//	100ms	20ms*5


	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_PLUSE_CFG, 0x60);
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_PLUSE_THSZ, 0x30);
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_PLUSE_TMLI, 0x0c);//60ms   5ms*12
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_PLUSE_LTCY, 0x14);//200ms	 10ms*20
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_PLUSE_WIND, 0x1e);//300ms	 10ms*30

	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_ASLP_COUNT, 0x00);
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG1, 0x20);
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG2, 0x0d);
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG3, 0x18);
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG4, 0x8c);		//interrupt disable
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG5, 0x0c);


	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_XYZ_DATA_CFG, 0x00);

	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG1, 0x21);

	return status;

}

int8 hal_msensor_8700_read_intsrc(void)
{
	int8 status = 0;		// I2C transaction status

	uint8 iWhoAmI;		// sensor WhoAmI

	uint8 int_source = 0;

	// check the WHOAMI register for the correct value and return immediately if invalid
	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_WHO_AM_I, &iWhoAmI, 1);
	status &= (iWhoAmI == FXOS8700_WHO_AM_I_VALUE) || (iWhoAmI == FXOS8700_WHO_AM_I_VALUE_ENG) || (iWhoAmI == FXOS8701_WHO_AM_I_VALUE);
	if (!status)
	{
		return HAL_MSENSOR_ERROR;
	}

	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_INT_SOURCE, &int_source, 1);
	if(int_source & FXOS8700_PLUSE_INT)
	{
		hal_msensor_clear_pluse_int();
		return FXOS8700_PLUSE_INT;
	}
	if(int_source & FXOS8700_MOTION_INT)
	{
		hal_msensor_clear_motion_int();
		return FXOS8700_MOTION_INT;
	}

	return int_source;
}

int8 hal_msensor_8700_close(void)
{
	int8 status;		// I2C transaction status
    uint8 iWhoAmI;		// sensor WhoAmI

    // check the WHOAMI register for the correct value and return immediately if invalid
	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_WHO_AM_I, &iWhoAmI, 1);

	// write 0000 0000 = 0x00 to CTRL_REG1 to place FXOS8700 into standby
	// [7-1] = 0000 000
	// [0]: active=0
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG1, 0x00);

	return status;
}

int8 hal_msensor_8700_open(void)
{
	int8 status;		// I2C transaction status
    uint8 iWhoAmI;		// sensor WhoAmI

    // check the WHOAMI register for the correct value and return immediately if invalid
	status = hal_msensor_read(FXOS8700_I2C_ADDR, FXOS8700_WHO_AM_I, &iWhoAmI, 1);


	// write 0000 1101 = 0x0D to accelerometer control register 1
	// [7-6]: aslp_rate=00
	// [5-3]: dr=001=1 for 200Hz data rate (when in hybrid mode)
	// [2]: lnoise=1 for low noise mode (since we're in 4g mode)
	// [1]: f_read=0 for normal 16 bit reads
	// [0]: active=1 to take the part out of standby and enable sampling
	status &= hal_msensor_write(FXOS8700_I2C_ADDR, FXOS8700_CTRL_REG1, 0x0D);

	return status;
}

int8 hal_msensor_21002_close(void)
{
	int8 status;		// I2C transaction status
    uint8 iWhoAmI;		// sensor WhoAmI

	// check the WHOAMI register for the correct value and return immediately if invalid
	status = hal_msensor_read(FXAS2100X_I2C_ADDR, FXAS2100X_WHO_AM_I, &iWhoAmI, 1);


	switch (iWhoAmI)
	{

	case FXAS21002_WHO_AM_I_VALUE:
	case FXAS21002_WHO_AM_I_VALUE_ENG:
		// write 0000 0000 = 0x00 to CTRL_REG1 to place FXOS21002 in Standby
		// [7]: ZR_cond=0
		// [6]: RST=0
		// [5]: ST=0 self test disabled
		// [4-2]: DR[2-0]=000 for 800Hz
		// [1-0]: Active=0, Ready=0 for Standby mode
		status &= hal_msensor_write(FXAS2100X_I2C_ADDR, FXAS2100X_CTRL_REG1, 0x00);

		break;
	default:
		// will never happen
		break;
	}

	return status;
}

int8 hal_msensor_21002_open(void)
{
	int8 status;		// I2C transaction status
    uint8 iWhoAmI;		// sensor WhoAmI
    int16 SENSORFS = 200;

	// check the WHOAMI register for the correct value and return immediately if invalid
	status = hal_msensor_read(FXAS2100X_I2C_ADDR, FXAS2100X_WHO_AM_I, &iWhoAmI, 1);

	switch (iWhoAmI)
	{

	case FXAS21002_WHO_AM_I_VALUE:
	case FXAS21002_WHO_AM_I_VALUE_ENG:

		// write 000X XX10 to CTRL_REG1 to configure ODR and enter Active mode
		// [7]: ZR_cond=0
		// [6]: RST=0
		// [5]: ST=0 self test disabled
		// [4-2]: DR[2-0]=000 for 800Hz ODR giving 0x02
		// [4-2]: DR[2-0]=001 for 400Hz ODR giving 0x06
		// [4-2]: DR[2-0]=010 for 200Hz ODR giving 0x0A
		// [4-2]: DR[2-0]=011 for 100Hz ODR giving 0x0E
		// [4-2]: DR[2-0]=100 for 50Hz ODR giving 0x12
		// [4-2]: DR[2-0]=101 for 25Hz ODR giving 0x16
		// [4-2]: DR[2-0]=110 for 12.5Hz ODR giving 0x1A
		// [4-2]: DR[2-0]=111 for 12.5Hz ODR giving 0x1E
		// [1-0]: Active=1, Ready=0 for Active mode
		if (SENSORFS >= 800)	// select 800Hz ODR
			status &= hal_msensor_write(FXAS2100X_I2C_ADDR, FXAS2100X_CTRL_REG1, 0x02);
		else if (SENSORFS >= 400) // select 400Hz ODR
			status &= hal_msensor_write(FXAS2100X_I2C_ADDR, FXAS2100X_CTRL_REG1, 0x06);
		else if (SENSORFS >= 200) // select 200Hz ODR
			status &= hal_msensor_write(FXAS2100X_I2C_ADDR, FXAS2100X_CTRL_REG1, 0x0A);
		else if (SENSORFS >= 100) // select 100Hz ODR
			status &= hal_msensor_write(FXAS2100X_I2C_ADDR, FXAS2100X_CTRL_REG1, 0x0E);
		else if (SENSORFS >= 50) // select 50Hz ODR
			status &= hal_msensor_write(FXAS2100X_I2C_ADDR, FXAS2100X_CTRL_REG1, 0x12);
		else if (SENSORFS >= 25)  // select 25Hz ODR
			status &= hal_msensor_write(FXAS2100X_I2C_ADDR, FXAS2100X_CTRL_REG1, 0x16);
		else // select 12.5Hz ODR
			status &= hal_msensor_write(FXAS2100X_I2C_ADDR, FXAS2100X_CTRL_REG1, 0x1A);
		break;
	default:
		// will never happen
		break;
	}

	return status;
}

// write a byte to specified I2C sensor and register
int8 hal_msensor_write(uint8 I2CAddress,uint8 I2CRegister, uint8 I2CData)
{
	uint8 res;
	uint8 I2C_Buffer[2];

	// set up the buffer and send (with stop sequence)
	I2C_Buffer[0] = I2CRegister;
	I2C_Buffer[1] = I2CData;

	if (I2CAddress == FXOS8700_I2C_ADDR)  // FXOS8700
	{
		res = ad_i2c_write(FXOS8700_dev, &I2C_Buffer[0], 2);
	}
	else if (I2CAddress == FXAS2100X_I2C_ADDR)	  // FXOS21002
	{
		res = ad_i2c_write(FXOS21002_dev, &I2C_Buffer[0], 2);
	}
	else
	{
		res = 0;
	}

	if(!res)
	{
		return HAL_MSENSOR_OK;
	}
	else
	{
		log_e("write error!addr:0x%x,reg:0x%x",I2CAddress,I2CRegister);
		return HAL_MSENSOR_ERROR;
	}
}

// read an array of bytes from a specified I2C sensor and start register
int8 hal_msensor_read(uint8 I2CAddress,uint8 I2CRegister, uint8 *I2C_Buffer, uint8 nbytes)
{
	int8 res;
	uint8 I2CRegister_Buff[1];

	I2CRegister_Buff[0] = I2CRegister;

	if (I2CAddress == FXOS8700_I2C_ADDR)  // FXOS8700
	{
		res = ad_i2c_transact(FXOS8700_dev, &I2CRegister_Buff[0], 1,
				I2C_Buffer, nbytes);
	}
	else if (I2CAddress == FXAS2100X_I2C_ADDR)	  // FXOS21002
	{
		res = ad_i2c_transact(FXOS21002_dev, &I2CRegister_Buff[0], 1,
				I2C_Buffer, nbytes);
	}
	else
	{
		res = 0;
	}

	if(!res)
	{
		return HAL_MSENSOR_OK;
	}
	else
	{
		log_e("read error!addr:0x%x,reg:0x%x",I2CAddress,I2CRegister);
		return HAL_MSENSOR_ERROR;
	}

}

void hal_msensor_reset(void)
{
	int8 res;

	res = hal_msensor_write(FXOS8700_I2C_ADDR,FXOS8700_CTRL_REG2,0x40);//reset
	res = hal_msensor_write(FXAS2100X_I2C_ADDR,FXAS2100X_CTRL_REG1,0x40);//reset

	OS_DELAY_MS(1);
}

void hal_gyro_reset(void)
{
    while (!hal_msensor_21002_close());
    while (!hal_msensor_21002_open());
    log_e("hal_gyro_reset!");
}

void hal_msensor_init(void)
{

	FXOS8700_dev = ad_i2c_open(FXOS8700);
	FXOS21002_dev = ad_i2c_open(FXOS21002);

	hal_msensor_reset();

}

