/*
 * hal_msensor.h
 *
 *  Created on: 2016Äê8ÔÂ5ÈÕ
 *      Author: Administrator
 */

#ifndef HAL_MSENSOR_H_
#define HAL_MSENSOR_H_

#include "sdk_defs.h"

#define FXOS8700_OUT_X_MSB       	  	0x01
#define FXOS8700_WHO_AM_I      			0x0D
#define FXOS8700_XYZ_DATA_CFG       	0x0E
#define FXOS8700_CTRL_REG1        	 	0x2A
#define FXOS8700_CTRL_REG2        	 	0x2B
#define FXOS8700_M_OUT_X_MSB       		0x33
#define FXOS8700_M_CTRL_REG1         	0x5B
#define FXOS8700_M_CTRL_REG2        	0x5C
#define FXOS8700_WHO_AM_I_VALUE_ENG     0xC4	// engineering samples
#define FXOS8700_WHO_AM_I_VALUE     	0xC7	// production
#define FXOS8701_WHO_AM_I_VALUE     	0xC9

#define FXOS8700_A_FFMT_CFG				0x15
#define FXOS8700_A_FFMT_SRC				0x16
#define FXOS8700_A_FFMT_THS				0x17
#define FXOS8700_A_FFMT_COUNT			0x18
#define FXOS8700_CTRL_REG3				0x2C
#define FXOS8700_CTRL_REG4        	 	0x2D
#define FXOS8700_CTRL_REG5        	 	0x2E
#define FXOS8700_INT_SOURCE        	 	0x0C



#define FXOS8700_PLUSE_CFG				0x21
#define FXOS8700_PLUSE_SRC				0x22
#define FXOS8700_PLUSE_THSX				0x23
#define FXOS8700_PLUSE_THSY				0x24
#define FXOS8700_PLUSE_THSZ       	 	0x25
#define FXOS8700_PLUSE_TMLI				0x26
#define FXOS8700_PLUSE_LTCY				0x27
#define FXOS8700_PLUSE_WIND				0x28

#define FXOS8700_ASLP_COUNT  			0x29
#define FXOS8700_SYSMOD					0x0B


// FXAS2100X registers and constants
#define FXAS2100X_DATA_REG            	0x01
#define FXAS2100X_WHO_AM_I        		0x0C
#define FXAS2100X_CTRL_REG0           	0x0D
#define FXAS2100X_CTRL_REG1           	0x13
#define FXAS21000_WHO_AM_I_VALUE		0xD1	// engineering and production
#define FXAS21002_WHO_AM_I_VALUE_ENG	0xD6	// engineering samples
#define FXAS21002_WHO_AM_I_VALUE		0xD7	// production

#define FXOS8700_I2C_ADDR				0x1E
#define FXAS2100X_I2C_ADDR				0x20

#define FXOS8700_PLUSE_INT				0x08
#define FXOS8700_MOTION_INT				0x04


#define HAL_MSENSOR_OK		(1)
#define HAL_MSENSOR_ERROR	(0)

int8 hal_msensor_8700_close(void);
int8 hal_msensor_8700_open(void);
int8 hal_msensor_21002_close(void);
int8 hal_msensor_21002_open(void);
int8 hal_msensor_set_detect_mode(void);
int8 hal_msensor_8700_read_intsrc(void);
void hal_msensor_reset(void);
int8 hal_msensor_write(uint8 I2CAddress,uint8 I2CRegister, uint8 I2CData);
int8 hal_msensor_read(uint8 I2CAddress,uint8 I2CRegister, uint8 *I2C_Buffer, uint8 nbytes);
void hal_gyro_reset(void);
void hal_msensor_init(void);

#endif
