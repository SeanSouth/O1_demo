// Copyright (c) 2014, 2015, Freescale Semiconductor, Inc.

// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Freescale Semiconductor, Inc. nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL FREESCALE SEMICONDUCTOR, INC. BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// This file defines real-time tasks required by the sensor fusion application.
// These are:
// * main task (one time startup)
// * sampling task (200 Hz nominal)
// * sensor fusion task (25Hz nominal)
// * magnetic calibration (background)
// These functions are intertwined with MQXLITE.  If you change RTOSs, you will have
// to rework this file.
//
#include <stdio.h>
#include <stdlib.h>

#include "build.h"
#include "stdint.h"
#include "mqx_tasks.h"

/* User includes (#include below this line is not maintained by Processor Expert) */
#include "types.h"
#include "fusion.h"
#include "magnetic.h"
#include "drivers.h"
#include "user_tasks.h"

#include "..\..\HAL\hal_msensor.h"

#define I2C_DeviceData  0

// global structures
struct ProjectGlobals globals;

extern void _lwevent_set(void);
extern void _lwevent_wait(void);

// sensor data structures
#if defined USE_MPL3115
struct PressureSensor thisPressure;		// this pressure sensor
#endif
#if defined USE_FXOS8700 || defined USE_MMA8652 || defined USE_FXLS8952
struct AccelSensor thisAccel;			// this accelerometer
#endif
#if defined USE_FXOS8700 || defined USE_MAG3110
struct MagSensor thisMag;				// this magnetometer
struct MagCalibration thisMagCal;		// hard and soft iron magnetic calibration
struct MagneticBuffer thisMagBuffer;	// magnetometer measurement buffer
#endif
#if defined USE_FXAS2100X
struct GyroSensor thisGyro;				// this gyro
#endif
// 1DOF pressure structure
#if defined COMPUTE_1DOF_P_BASIC
struct SV_1DOF_P_BASIC thisSV_1DOF_P_BASIC;					
#endif
// 3DOF accelerometer (Basic) structure
#if defined COMPUTE_3DOF_G_BASIC
struct SV_3DOF_G_BASIC thisSV_3DOF_G_BASIC;
#endif
// 3DOF magnetometer (Basic) structure
#if defined COMPUTE_3DOF_B_BASIC
struct SV_3DOF_B_BASIC thisSV_3DOF_B_BASIC;					
#endif
// 3DOF gyro (Basic) structure
#if defined COMPUTE_3DOF_Y_BASIC
struct SV_3DOF_Y_BASIC thisSV_3DOF_Y_BASIC;					
#endif
// 6DOF accelerometer and magnetometer (Basic) structure
#if defined COMPUTE_6DOF_GB_BASIC
struct SV_6DOF_GB_BASIC thisSV_6DOF_GB_BASIC;					
#endif
// 6DOF accelerometer and gyro (Kalman) structure
#if defined COMPUTE_6DOF_GY_KALMAN
struct SV_6DOF_GY_KALMAN thisSV_6DOF_GY_KALMAN;
#endif
// 9DOF accelerometer, magnetometer and gyro (Kalman) structure
#if defined COMPUTE_9DOF_GBY_KALMAN
struct SV_9DOF_GBY_KALMAN thisSV_9DOF_GBY_KALMAN;
#endif

#if 0
#define a 50

char value;

char filter()
{
    char   new_value;
    new_value = get_ad();
    return (100-a)*value + a*new_value;
}

A、方法：
取a=0~1
本次滤波结果=（1-a）*上次滤波结果+a*本次采样值

B、优点：
对周期性干扰具有良好的抑制作用
适用于波动频率较高的场合
C、缺点：
相位滞后，灵敏度低
滞后程度取决于a值大小
不能消除滤波频率高于采样频率的1/2的干扰信号

D、a的选取，设滤波时间为t，采样频率为F则a＝1/tF
#endif

int16 last_iBs[3];							// 上一次的ibs值
uint8 mag_firsttime=1;
void filter_mag(struct MagSensor* theMag)
{
    int i;
    
	if(mag_firsttime==1)
	{
		for(i = CHX; i <= CHZ; i++)
		{
		    last_iBs[i] = theMag->iBs[i];
		}
		mag_firsttime=0;
	}
	else
	{
        for(i = CHX; i <= CHZ; i++)
        {
    	    theMag->iBs[i]= ((float)(1-MAG_FILTER_PARA)*last_iBs[i] + (float)MAG_FILTER_PARA*theMag->iBs[i]);
    	    last_iBs[i] = theMag->iBs[i];
        }
	}

    return ;
}

int16 last_iGs[3];							// most recent unaveraged measurement (counts)
uint8 accel_firsttime=1;
void filter_accel(struct AccelSensor* theAccel)
{
    int i;
    
	if(accel_firsttime==1)
	{
		for(i = CHX; i <= CHZ; i++)
		{
		    last_iGs[i] = theAccel->iGs[i];
		}
		accel_firsttime=0;
	}
	else
	{
        for(i = CHX; i <= CHZ; i++)
        {
    	    theAccel->iGs[i]= ((float)(1-ACCEL_FILTER_PARA)*last_iGs[i] + (float)ACCEL_FILTER_PARA*theAccel->iGs[i]);
    	    last_iGs[i] = theAccel->iGs[i];
        }
	}

    return ;
}

// sensor read task: this is interrupted by hardware callbacks (UART etc) but never by fusion or magnetic calibration
void RdSensData_task_init(void)
{
	int32 iSum[3];					// array of sums
	int32 itmp;						// scratch
	static int8 iCounter = 0;		// decimation counter range 0 to OVERSAMPLE_RATIO-1
	int8 i, j, k, l;				// counters

	// initialize globals
	globals.AngularVelocityPacketOn = true;
	globals.DebugPacketOn = true;
	globals.RPCPacketOn = true;
	globals.AltPacketOn = true;

	// application-specific startup
	UserStartup();  

	// initialize the physical sensors over I2C and hang on error to keep the red LED illuminated
	// with exception of MPL3115 (so as to support 9AXIS board option which doesn't populate MPL3115)


	FXOS8700_Init(0, &thisAccel, &thisMag);
	//FXOS8700_Init_Standby(0);
#if defined USE_FXAS2100X
	while (!FXAS2100X_Init(0, &thisGyro));
#endif


	// initialize magnetometer data structure
#if defined COMPUTE_3DOF_B_BASIC || defined COMPUTE_6DOF_GB_BASIC || defined COMPUTE_9DOF_GBY_KALMAN
	// zero the calibrated measurement since this is used for indexing the magnetic buffer even before first calibration
	for (i = CHX; i <= CHZ; i++)
		thisMag.iBcAvg[i]= 0;
#endif

    //auto-sleep
    while (!hal_msensor_21002_close());
    while (!hal_msensor_8700_close());
        
	// infinite loop controlled by hardware timer (typically 200Hz) with iCounter in range 0 to OVERSAMPLE_RATIO-1
    // moved to run once
}

void RdSensData_task_runonece(void)
{
	int32 iSum[3];					// array of sums
	int32 itmp;						// scratch
	static int8 iCounter = 0;		// decimation counter range 0 to OVERSAMPLE_RATIO-1
	int8 i, j, k, l;				// counters

	// wait here for the sensor sampling event from the hardware timer
	// the sensor fusion and magnetic calibration tasks execute while this task is blocked here
	// FALSE means any bit (of the 1 bit enabled by the mask) unblocks and NULL means infinite timeout

	// read and process accelerometer sensor in every slot if present if accelerometer algorithm is in use.
#if defined COMPUTE_3DOF_G_BASIC || defined COMPUTE_6DOF_GB_BASIC || defined COMPUTE_6DOF_GY_KALMAN || defined COMPUTE_9DOF_GBY_KALMAN
#if defined USE_FXOS8700
	FXOS8700_ReadAccData(0, &thisAccel);
    #if (USE_LOWPASS_FILTER==1)
            filter_accel(&thisAccel);
    #endif
        
#endif
#if defined USE_MMA8652
	MMA8652_ReadData(I2C_DeviceData, &thisAccel);
#endif
#if defined USE_FXLS8952
	FXLS8952_ReadData(I2C_DeviceData, &thisAccel);
#endif
	// store measurement in a buffer for later end of block processing
	for (i = CHX; i <= CHZ; i++)
		thisAccel.iGsBuffer[iCounter][i] = thisAccel.iGs[i];

	// every OVERSAMPLE_RATIO passes calculate the block averaged measurement
	if (iCounter == (OVERSAMPLE_RATIO - 1))
	{	
		// calculate the block averaged measurement in counts and g
		for (i = CHX; i <= CHZ; i++)
		{
			iSum[i] = 0;
			for (j = 0; j < OVERSAMPLE_RATIO; j++)
				iSum[i] += (int32)thisAccel.iGsBuffer[j][i];
			// compute the average with nearest integer rounding
			if (iSum[i] >= 0)
				thisAccel.iGsAvg[i] = (int16)((iSum[i] + (OVERSAMPLE_RATIO >> 1)) / OVERSAMPLE_RATIO);
			else
				thisAccel.iGsAvg[i] = (int16)((iSum[i] - (OVERSAMPLE_RATIO >> 1)) / OVERSAMPLE_RATIO);
			// convert from integer counts to float g
			thisAccel.fGsAvg[i] = (float)thisAccel.iGsAvg[i] * thisAccel.fgPerCount;
		}
	} // end of test for end of OVERSAMPLE_RATIO block
#endif // end of check for accelerometer algorithm and sensor

	// read and process the magnetometer sensor in every slot if magnetic algorithm is in use.
#if defined COMPUTE_3DOF_B_BASIC || defined COMPUTE_6DOF_GB_BASIC || defined COMPUTE_9DOF_GBY_KALMAN
#if defined USE_FXOS8700
	FXOS8700_ReadMagData(0, &thisMag);
    #if (USE_LOWPASS_FILTER==1)
        filter_mag(&thisMag);
    #endif
#endif
#if defined USE_MAG3110
	MAG3110_ReadData(I2C_DeviceData, &thisMag);
#endif
	// store in a buffer for later end of block processing
	for (i = CHX; i <= CHZ; i++)
		thisMag.iBsBuffer[iCounter][i] = thisMag.iBs[i];

	// update magnetic buffer with iBs avoiding a write to the shared structure while a calibration is in progress.
	if (!thisMagCal.iCalInProgress)
		iUpdateMagnetometerBuffer(&thisMagBuffer, &thisMag, globals.loopcounter);

	// every OVERSAMPLE_RATIO passes calculate the block averaged and calibrated measurement using an anti-glitch filter
	// that rejects the measurement furthest from the mean. magnetometer sensors are sensitive
	// to occasional current pulses from power supply traces and so on and this is a simple method to remove these.
	if (iCounter == (OVERSAMPLE_RATIO - 1))
	{
		// calculate the channel means using all measurements
		for (i = CHX; i <= CHZ; i++)
		{
			// accumulate channel sums
			iSum[i] = 0;
			for (j = 0; j < OVERSAMPLE_RATIO; j++)
				iSum[i] += (int32)thisMag.iBsBuffer[j][i];
		}
		// store axis k in buffer measurement l furthest from its mean
		itmp = 0;
		for (i = CHX; i <= CHZ; i++)
		{
			for (j = 0; j < OVERSAMPLE_RATIO; j++)
			{
				if (abs((int32)thisMag.iBsBuffer[j][i] * OVERSAMPLE_RATIO - iSum[i]) >= itmp)
				{
					k = i;
					l = j;
					itmp = abs((int32)thisMag.iBsBuffer[j][i] * OVERSAMPLE_RATIO - iSum[i]);
				}
			}
		}

		// re-calculate the block averaged measurement ignoring channel k in measurement l
		if (OVERSAMPLE_RATIO == 1)
		{
			// use the one available measurement for averaging in this case
			for (i = CHX; i <= CHZ; i++)
			{
				thisMag.iBsAvg[i] = thisMag.iBsBuffer[0][i];
			}
		} // end of compute averages for OVERSAMPLE_RATIO = 1
		else
		{
			// sum all measurements ignoring channel k in measurement l
			for (i = CHX; i <= CHZ; i++)
			{
				iSum[i] = 0;
				for (j = 0; j < OVERSAMPLE_RATIO; j++)
				{
					if (!((i == k) && (j == l)))
						iSum[i] += (int32)thisMag.iBsBuffer[j][i];
				}
			}
			// compute the average with nearest integer rounding
			for (i = CHX; i <= CHZ; i++)
			{
				if (i != k)
				{
					// OVERSAMPLE_RATIO measurements were used
					if (iSum[i] >= 0)
						thisMag.iBsAvg[i] = (int16)((iSum[i] + (OVERSAMPLE_RATIO >> 1)) / OVERSAMPLE_RATIO);
					else
						thisMag.iBsAvg[i] = (int16)((iSum[i] - (OVERSAMPLE_RATIO >> 1)) / OVERSAMPLE_RATIO);
				}
				else
				{
					// OVERSAMPLE_RATIO - 1 measurements were used
					if (iSum[i] >= 0)
						thisMag.iBsAvg[i] = (int16)((iSum[i] + ((OVERSAMPLE_RATIO - 1) >> 1)) / (OVERSAMPLE_RATIO - 1));
					else
						thisMag.iBsAvg[i] = (int16)((iSum[i] - ((OVERSAMPLE_RATIO - 1) >> 1)) / (OVERSAMPLE_RATIO - 1));
				}
			}
		} // end of compute averages for OVERSAMPLE_RATIO = 1

		// convert the averages to float
		for (i = CHX; i <= CHZ; i++)
			thisMag.fBsAvg[i] = (float)thisMag.iBsAvg[i] * thisMag.fuTPerCount;

		// remove hard and soft iron terms from fBsAvg (uT) to get calibrated data fBcAvg (uT), iBc (counts)
		fInvertMagCal(&thisMag, &thisMagCal);
		
	} // end of test for end of OVERSAMPLE_RATIO block
#endif // end of check for magnetic algorithms and sensor

	// read the gyro sensor every time slot
#if defined COMPUTE_3DOF_Y_BASIC || defined COMPUTE_6DOF_GY_KALMAN || defined COMPUTE_9DOF_GBY_KALMAN
#if defined USE_FXAS2100X
	FXAS2100X_ReadData(I2C_DeviceData, &thisGyro);
#endif
	// store in a buffer for later gyro integration by sensor fusion algorithms
	for (i = CHX; i <= CHZ; i++)
		thisGyro.iYsBuffer[iCounter][i] = thisGyro.iYs[i];
#endif // end of check for gyro algorithm and sensor

	// read pressure only in first iCounter slot giving read at SENSORFS/OVERSAMPLE_RATIO Hz = 25Hz nominal
	// the MPL3115 is set to average 512 internal samples with 512ms ODR but it is read here at (typically) 25Hz
	// to over-sample and allow the low pass filter in the fusion task to smoothly interpolate the measurement.
#if defined USE_MPL3115
	if ((iCounter == 0) && thisPressure.iWhoAmI)
		MPL3115_ReadData(I2C_DeviceData, &thisPressure);
#endif // end of check for pressure sensor

	// run the user high frequency task
	UserHighFrequencyTaskRun();

	// every OVERSAMPLE_RATIO passes zero the decimation counter and enable the sensor fusion task
	if (iCounter++ == (OVERSAMPLE_RATIO - 1))
	{
		iCounter = 0;
	} 
}

// sensor fusion task: this is continually interrupted by the high priority sensor read task
void Fusion_task_init(void)
{

	// initialize the sensor fusion algorithms
	fInitFusion();

	// initialize the user medium frequency (typically 25Hz) task
	UserMediumFrequencyTaskInit();

}

void Fusion_task_runonce(void)
{
	int8 initiatemagcal;				// flag to initiate a new magnetic calibration
    extern void _lwevent_set(void);

    // wait for the sensor fusion task to be enabled
    // the magnetic calibration task executes while the fusion task is blocked here
    //_lwevent_wait_for(&(globals.RunKFEventStruct), 1, FALSE, NULL);

    // 1DOF Pressure: call the low pass filter algorithm
    #if defined COMPUTE_1DOF_P_BASIC
    thisSV_1DOF_P_BASIC.systick = SYST_CVR & 0x00FFFFFF;
    fRun_1DOF_P_BASIC(&thisSV_1DOF_P_BASIC, &thisPressure);
    thisSV_1DOF_P_BASIC.systick -= SYST_CVR & 0x00FFFFFF;
    if (thisSV_1DOF_P_BASIC.systick < 0) thisSV_1DOF_P_BASIC.systick += SYST_RVR;	
    #endif

    // 3DOF Accel Basic: call the tilt algorithm
    #if defined COMPUTE_3DOF_G_BASIC		
    thisSV_3DOF_G_BASIC.systick = SYST_CVR & 0x00FFFFFF;		
    fRun_3DOF_G_BASIC(&thisSV_3DOF_G_BASIC, &thisAccel);
    thisSV_3DOF_G_BASIC.systick -= SYST_CVR & 0x00FFFFFF;
    if (thisSV_3DOF_G_BASIC.systick < 0) thisSV_3DOF_G_BASIC.systick += SYST_RVR;
    #endif

    // 3DOF Magnetometer Basic: call the 2D vehicle compass algorithm
    #if defined COMPUTE_3DOF_B_BASIC
    thisSV_3DOF_B_BASIC.systick = SYST_CVR & 0x00FFFFFF;
    fRun_3DOF_B_BASIC(&thisSV_3DOF_B_BASIC, &thisMag);
    thisSV_3DOF_B_BASIC.systick -= SYST_CVR & 0x00FFFFFF;
    if (thisSV_3DOF_B_BASIC.systick < 0) thisSV_3DOF_B_BASIC.systick += SYST_RVR;	
    #endif

    // 3DOF Gyro Basic: call the gyro integration algorithm
    #if defined COMPUTE_3DOF_Y_BASIC	
    thisSV_3DOF_Y_BASIC.systick = SYST_CVR & 0x00FFFFFF;
    fRun_3DOF_Y_BASIC(&thisSV_3DOF_Y_BASIC, &thisGyro);
    thisSV_3DOF_Y_BASIC.systick -= SYST_CVR & 0x00FFFFFF;
    if (thisSV_3DOF_Y_BASIC.systick < 0) thisSV_3DOF_Y_BASIC.systick += SYST_RVR;
    #endif

    // 6DOF Accel / Mag: Basic: call the eCompass orientation algorithm
    #if defined COMPUTE_6DOF_GB_BASIC		
    thisSV_6DOF_GB_BASIC.systick = SYST_CVR & 0x00FFFFFF;
    fRun_6DOF_GB_BASIC(&thisSV_6DOF_GB_BASIC, &thisMag, &thisAccel);
    thisSV_6DOF_GB_BASIC.systick -= SYST_CVR & 0x00FFFFFF;
    if (thisSV_6DOF_GB_BASIC.systick < 0) thisSV_6DOF_GB_BASIC.systick += SYST_RVR;		
    #endif

    // 6DOF Accel / Gyro: call the Kalman filter orientation algorithm
    #if defined COMPUTE_6DOF_GY_KALMAN		
    thisSV_6DOF_GY_KALMAN.systick = SYST_CVR & 0x00FFFFFF;
    fRun_6DOF_GY_KALMAN(&thisSV_6DOF_GY_KALMAN, &thisAccel, &thisGyro);
    thisSV_6DOF_GY_KALMAN.systick -= SYST_CVR & 0x00FFFFFF;
    if (thisSV_6DOF_GY_KALMAN.systick < 0) thisSV_6DOF_GY_KALMAN.systick += SYST_RVR;	
    #endif
    // 9DOF Accel / Mag / Gyro: call the Kalman filter orientation algorithm
    #if defined COMPUTE_9DOF_GBY_KALMAN		
    //thisSV_9DOF_GBY_KALMAN.systick = SYST_CVR & 0x00FFFFFF;
    fRun_9DOF_GBY_KALMAN(&thisSV_9DOF_GBY_KALMAN, &thisAccel, &thisMag, &thisGyro, &thisMagCal);
    //thisSV_9DOF_GBY_KALMAN.systick -= SYST_CVR & 0x00FFFFFF;		
    //if (thisSV_9DOF_GBY_KALMAN.systick < 0) thisSV_9DOF_GBY_KALMAN.systick += SYST_RVR;	
    #endif

    // decide whether or not to initiate a magnetic calibration
    #if defined COMPUTE_3DOF_B_BASIC || defined COMPUTE_6DOF_GB_BASIC || defined COMPUTE_9DOF_GBY_KALMAN
    // check no magnetic calibration is in progress
    if (!thisMagCal.iCalInProgress)
    {
    	// do the first 4 element calibration immediately there are a minimum of MINMEASUREMENTS4CAL
    	initiatemagcal = (!thisMagCal.iMagCalHasRun && (thisMagBuffer.iMagBufferCount >= MINMEASUREMENTS4CAL));

    	// otherwise initiate a calibration at intervals depending on the number of measurements available
    	initiatemagcal |= ((thisMagBuffer.iMagBufferCount >= MINMEASUREMENTS4CAL) && 
    			(thisMagBuffer.iMagBufferCount < MINMEASUREMENTS7CAL) &&
    			!(globals.loopcounter % INTERVAL4CAL));
    	initiatemagcal |= ((thisMagBuffer.iMagBufferCount >= MINMEASUREMENTS7CAL) &&
    			(thisMagBuffer.iMagBufferCount < MINMEASUREMENTS10CAL) &&
    			!(globals.loopcounter % INTERVAL7CAL));
    	initiatemagcal |= ((thisMagBuffer.iMagBufferCount >= MINMEASUREMENTS10CAL) &&
    			!(globals.loopcounter % INTERVAL7CAL));//leran modify

    	// initiate the magnetic calibration if any of the conditions are met
    	if (initiatemagcal)
    	{
    		//_lwevent_set(&(globals.MagCalEventStruct), 1);
            _lwevent_set();
    	}

    } // end of test that no calibration is already in progress
    #endif

    // increment the loopcounter (used for time stamping magnetic data)
    globals.loopcounter++;

    // run the user medium frequency (typically 25Hz) user task
    UserMediumFrequencyTaskRun();

}

// magnetic calibration task: this has the lowest priority and is continually interrupted by sensor read and fusion tasks
void MagCal_task_init(void)
{	
	// initialize magnetic calibration and magnetometer data buffer
#if defined COMPUTE_3DOF_B_BASIC || defined COMPUTE_6DOF_GB_BASIC || defined COMPUTE_9DOF_GBY_KALMAN
	fInitMagCalibration(&thisMagCal, &thisMagBuffer);
#endif

}

void MagCal_task_runonce(void)
{	
    extern void _lwevent_wait(void);
    
    _lwevent_wait();
	// run the magnetic calibration
#if defined COMPUTE_3DOF_B_BASIC || defined COMPUTE_6DOF_GB_BASIC || defined COMPUTE_9DOF_GBY_KALMAN
	thisMagCal.iCalInProgress = true;
	thisMagCal.iMagCalHasRun = true;
	fRunMagCalibration(&thisMagCal, &thisMagBuffer, &thisMag);
#endif
}
