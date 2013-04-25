///////////////////////////////////////////////////////////////////////////////
// handcontrolthread.cpp - Interface to control and receive feedback from the hand
//
// Copyright (C) 2012, Neurolutions
//
// Author: Rob Coker <rcoker@pythagdev.com>
//
///////////////////////////////////////////////////////////////////////////////

#include <QEventLoop>
#include "handcontrolthread.h"

#ifndef Q_OS_WIN
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <cstdlib>


// Implementation notes:
// Battery is on ADCIN3
// Finger Position feedbacks are on ADCIN2 & ADCIN7

// pwm can operate in 5 - 10k range for hardware, requires 1 ms between changing direction and applying drive, so:
// Make pwm value = 0
// wait a few msec
// change dir gpio
// wait a few msec
// set pwm value to desired

// finger motor control has a PWM (magnitude) and GPIO (direction) with the following mapping:
// NOTE: this may not be final values - check updated schematic
// Finger 1, PWM8 & GPIO7
// Finger 2, PWM9 & GPIO21

/// names of built-in PWM devices
const char PWM_DEVICES[2][20] = {"/dev/pwm8", "/dev/pwm9"};

/// names of gpio
const char GPIO_DEVICES[NUM_FINGERS][40] = {"/sys/class/gpio/gpio17/value",
                                "/sys/class/gpio/gpio21/value",
                                };
                                
/// name of the device for getting ADCIN2 and ADCIN7 from the SOM (finger positions)
const char ADC_FINGER_POS_DEVICE[] = "/sys/class/hwmon/hwmon0/device/in2_and_7_input";

/// name of the device for getting ADCIN3 from the SOM (battery level)
const char ADC_BATTERY_DEVICE[] = "/sys/class/hwmon/hwmon0/device/in3_input";

HandControlThread::HandControlThread(QObject *parent) :
    QThread(parent)
{
    for (int i = 0; i < NUM_FINGERS; i++)
    {
        fingerDirs[i] = FINGER_DIR_OPEN;
        fingerPwmLevel[i] = 0.0f;
        currPositionSample[i] = 0;
        batteryLevel = 0;
        
        pwmFileDescriptors[i] = -1;
        
        pwmState[i] = PWM_NORMAL;
    }
 }

HandControlThread::~HandControlThread()
{
}

bool HandControlThread::startThread()
{
	if (isRunning())
		return false;

#ifdef Q_OS_WIN
	qDebug("Opening pwm file handles");
#else
    // open pwm files
    for (int i = 0; i < NUM_FINGERS; i++)
    {
        pwmFileDescriptors[i] = open(PWM_DEVICES[i], O_RDWR);
        if (pwmFileDescriptors[i] < 0)
        {
            qDebug("HandControlThread::run: Could not open %s", PWM_DEVICES[i]);
			closeFiles();
			return false;
        }
    }
 #endif
	m_done = false;

	start();

    qDebug("HandControlThread started");

	return true;
}

void HandControlThread::stopThread()
{
	int i;

	m_done = true;

	for (i = 0; i < 10; i++) {
		wait(100);

		if (!isRunning())
			break;
	}

	if (i == 10)
		qDebug("Failed to stop HandControlThread");
	
	for (i = 0; i < NUM_FINGERS; i++)
		SetPwmForFinger(0, i);

	closeFiles();
}

void HandControlThread::closeFiles()
{
	for (int i = 0; i < NUM_FINGERS; i++) {
		SetPwmForFinger(0, i);
#ifdef Q_OS_WIN
		qDebug("Closing pwm file handles");
#else
		if (pwmFileDescriptors[i] >= 0) {
			close(pwmFileDescriptors[i]);
			pwmFileDescriptors[i] = -1;
		}
#endif
	}
}
    

/// set the drive level and implied direction
/// iDriveLevel should be -100 - 100 where negative implies opening
void HandControlThread::SetFingerDrive(qint16 iDriveLevel[NUM_FINGERS])
{
    dataMutex.lock();
    controlMutex.lock();
    
    for (int i = 0; i < NUM_FINGERS; i++)
    {
        FingerDir inFingerDir = FINGER_DIR_CLOSE;
        if (iDriveLevel[i] > 0)
        {
            inFingerDir = FINGER_DIR_OPEN;
        }
        
        qint16 inPwmValue = (qint16) abs(iDriveLevel[i]);
        
        // handle the cases:
        // 1. No change - do nothing
        // 2a. Value changed, but not sign and not in dir change- change pwm level
        // 2b. Value changed, already in dir change - change target pwm, but not actual level
        // 3. Sign changed - go to PRE_WAIT_TO_CHANGE_DIR
        if ((inPwmValue != fingerPwmLevel[i]) || (inFingerDir != fingerDirs[i]))
        {
            if (inFingerDir == fingerDirs[i])
            {
                // value only has changed
                fingerPwmLevel[i] = inPwmValue;
                
                // only set a new pwm value if we're in normal state, otherwise let the direction
                // change process run its course
                if (pwmState[i] == PWM_NORMAL)
                {
                    SetPwmForFinger(abs(fingerPwmLevel[i]), i);
                }
            }
            else
            {
                // sign has changed
                fingerDirs[i] = inFingerDir;
                fingerPwmLevel[i] = inPwmValue;
                pwmState[i] = PRE_WAIT_TO_CHANGE_DIR;
                // turn PWM off when we change direction to avoid a short-circuit in H-topology
                SetPwmForFinger(0, i);
            }
        }
        // else do nothing
    }
    
    controlMutex.unlock();
    dataMutex.unlock();
}
/*
/// gets the currently targeted drive levels
/// oDriveLevel is a pointer to a NUM_FINGER long array to write to
void HandControlThread::GetFingerDriveLevel(int16_t* oDriveLevel)
{
    dataMutex.lock();
    
    dataMutex.unlock();
}

/// gets the currently targeted finger directions
/// oFingerDir is a pointer to a NUM_FINGER long array to write to
void HandControlThread::GetFingerDir(FingerDir* oFingerDir)
{
    dataMutex.lock();
    dataMutex.unlock();
}
*/

/// gets the battery level
/// return value is 0 - 100 as a percent of full
quint16 HandControlThread::GetBatteryLevel()
{
    quint16 lBattLvl;
    dataMutex.lock();
    lBattLvl = batteryLevel;
    dataMutex.unlock();
    
    return lBattLvl;
}

/// gets the current finger position
/// oFingerPos is a pointer to a NUM_FINGER long array to write to
/// each value is 0 - 100 where 100 is fully extended and 0 is fully closed
void HandControlThread::GetFingerPos(quint16* oFingerPos)
{
    dataMutex.lock();
    for (int i = 0; i < NUM_FINGERS; i++)
    {
        oFingerPos[i] = currPositionSample[i];
    }
    
    dataMutex.unlock();
}


void HandControlThread::SetFingerPos(quint16* iFingerPos)
{
    dataMutex.lock();
    for (int i = 0; i < NUM_FINGERS; i++)
    {
        currPositionSample[i] = iFingerPos[i];
    }
    
    dataMutex.unlock();
    
    emit fingerPositionUpdated();
}

void HandControlThread::SetBatteryLevel(quint16 iBatteryLevel)
{
    dataMutex.lock();
    batteryLevel = iBatteryLevel;
    dataMutex.unlock();
    
    emit batteryLevelUpdated();
}

void HandControlThread::SetPwmForFinger(int iValue, int iFingerNum)
{
    // use the built-in PWMs
    char buf[10];
    sprintf(buf, "%d", iValue);

#ifdef Q_OS_WIN
	if (iValue == 0)
		qDebug("Turning off pwm for finger %d", iFingerNum);
	else
		qDebug("Writing %d to pwm for finger %d", iValue, iFingerNum);
#else
/*    
    ssize_t writeRet = write(pwmFileDescriptors[iFingerNum], buf, strlen(buf));
    if (writeRet < 0)
    {
        qDebug("HandControlThread::SetPwmForFinger Error Writing, errno = %d", errno);
    }
*/
	if (iValue == 0)
		qDebug("DEBUG: Would be turning off pwm for finger %d", iFingerNum);
	else
		qDebug("DEBUG: Would be writing %d to pwm for finger %d", iValue, iFingerNum);
#endif
}

void HandControlThread::SetDirForFinger(FingerDir iFingerDir, int iFingerNum)
{
	int fd;

    if (iFingerNum < NUM_FINGERS)
    {
        char value;
        if (FINGER_DIR_OPEN == iFingerDir)
        {
            value = '1';
        }
        else
        {
            value = '0';
        }

#ifndef Q_OS_WIN
        fd = open(GPIO_DEVICES[iFingerNum], O_RDWR);

        if (fd < 0)
		{
            qDebug("HandControlThread::run: Could not open %s", GPIO_DEVICES[iFingerNum]);
			return;
        }
#endif

		qDebug("Writing %c to gpio for finger %d", value, iFingerNum);

#ifndef Q_OS_WIN
        ssize_t writeRet = write(fd, &value, 1);
        if (writeRet < 0)
        {
            qDebug("HandControlThread::SetDirForFinger Error Writing, errno = %d", errno);
        }

		close(fd);
#endif
    }
    else
    {
        qDebug("HandControlThread::SetPwmForFinger: Invalid Finger Index = %d", iFingerNum);
    }
}

void HandControlThread::UpdatePwmControlStates()
{
    dataMutex.lock();
    controlMutex.lock();

    for (int i = 0; i < NUM_FINGERS; i++)
    {
        switch (pwmState[i])
        {
            case (PWM_NORMAL):
                // do nothing - pwm value will be set in SetFingerDrive
                break;
            case (PRE_WAIT_TO_CHANGE_DIR):
                qDebug("PRE_WAIT_TO_CHANGE_DIR: finger%d", i);
                pwmState[i] = RXED_WAIT_TO_CHANGE_DIR;
                break;
            case (RXED_WAIT_TO_CHANGE_DIR):
                qDebug("RXED_WAIT_TO_CHANGE_DIR: finger%d", i);
                pwmState[i] = PRE_WAIT_TO_SET_PWR;
                SetDirForFinger(fingerDirs[i], i);
                break;
            case (PRE_WAIT_TO_SET_PWR):
                qDebug("PRE_WAIT_TO_SET_PWR: finger%d", i);
                pwmState[i] = PWM_NORMAL;
                SetPwmForFinger(fingerPwmLevel[i], i);
                break;
        }
    }
    
    controlMutex.unlock();
    dataMutex.unlock();
}

void HandControlThread::run()
{      
    // use a loop count as a way of managing periodic tasks (let them run ever so-many loops)
    int loopCount = 0;
    
    while (!m_done)
    {
        // make total eventloop time around 10 msec, but split in two for pwm processing
        // NOTE: if event loop time changes, several of the loop count values below should be 
        // adjusted accordingly
		msleep(5);
        UpdatePwmControlStates();
        msleep(5);
        UpdatePwmControlStates();
        
        // get finger positions one-third as often (around 33 Hz)
        if ((loopCount % 3) == 0)
        {
			ReadFingerPositions();
        }
        
        // get battery level around once/second
        if ((loopCount % 99) == 0)
        {
			ReadBatteryLevel();
        }
        
        // reset the loop count when it would be 99 the next round
        loopCount++;
        if (loopCount >= 99)
        {
            loopCount = 0;
        }
    }    
}

void HandControlThread::ReadFingerPositions()
{
	quint16 samples[2];

#ifdef Q_OS_WIN
	for (int i = 0; i < 2; i++) {
		if (fingerPwmLevel[i] == 0) {
			samples[i] = currPositionSample[i];
		}
		else {
			if (fingerDirs[i] == FINGER_DIR_OPEN) {
				if (currPositionSample[i] <= 0)
					samples[i] = 0;
				else
					samples[i] = currPositionSample[i] - 1;
			}
			else {
				if (currPositionSample[i] >= 100)
					samples[i] = 100;
				else
					samples[i] = currPositionSample[i] + 1;
			}
		}
	}

	SetFingerPos(samples);
#else
	char buff[25];

    int fd = open(ADC_FINGER_POS_DEVICE, O_RDONLY);

    if (fd < 0)
    {
        qDebug("HandControlThread::run: Could not open %s", ADC_FINGER_POS_DEVICE);
		return;
    }

	memset(buff, 0, sizeof(buff));

	if (read(fd, buff, 25) < 0)
	{
		qDebug("HandControlThread: error reading adc in 2 & 7, errno = %d", errno);
	}
	else
	{
		sscanf(buff, "%hd %hd", &samples[0], &samples[1]);
		SetFingerPos(samples);             
	}

	close(fd);
#endif
}

void HandControlThread::ReadBatteryLevel()
{
	quint16 sample;

#ifdef Q_OS_WIN
	if (batteryLevel == 0)
		sample = 100;
	else
		sample = batteryLevel - 1;

	SetBatteryLevel(sample);
#else
	char buff[10];

	int fd = open(ADC_BATTERY_DEVICE, O_RDONLY);

    if (fd < 0)
    {
        qDebug("HandControlThread::run: Could not open %s", ADC_BATTERY_DEVICE);
		return;
    }
    
	memset(buff, 0, sizeof(buff));

	if (read(fd, buff, 10) < 0)
	{
		qDebug("HandControlThread: error reading adc in 3, errno = %d", errno);
	}
	else
	{
		sscanf(buff, "%hd", &sample);
		SetBatteryLevel(sample);
	}

	close(fd);
#endif
}
