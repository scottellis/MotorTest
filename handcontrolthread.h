///////////////////////////////////////////////////////////////////////////////
// handcontrolthread.h - Interface to control and receive feedback from the hand
//
// Copyright (C) 2012, Neurolutions
//
// Author: Rob Coker <rcoker@pythagdev.com>
//
///////////////////////////////////////////////////////////////////////////////

#ifndef HandControl_h
#define HandControl_h

#include <QThread>
#include <QMutex>

/// number of fingers that can be independently driven and read
const int NUM_FINGERS = 2;

/// Enum for the direction output that is coupled with the PWM level to drive
/// a finger
enum FingerDir
{
    FINGER_DIR_OPEN,
    FINGER_DIR_CLOSE
};

/// The HandControlThread class provides control over the hand's 2 motors and feedback 
/// from the hand's 2 position sensors.
/// The values handled are:
/// TPS65950 (accessed via I2C): 
///     ADCIN2, ADCIN7 for position sensors 
///     ADCIN3 for battery voltage
// TODO: fix this when known
/// PWM8, 9, 11: fingers 1 - 3
/// GPIO (2): direction bits for motor drives

class HandControlThread : public QThread
{
    Q_OBJECT
public:    
    explicit HandControlThread(QObject *parent = 0);
    ~HandControlThread();
    
	bool startThread();
	void stopThread();

    /// set the drive level and implied direction
    /// iDriveLevel should be -100 - 100 where negative implies opening
    void SetFingerDrive(qint16 iDriveLevel[NUM_FINGERS]);
    
    /// gets the battery level
    /// return value is 0 - 100 as a percent of full
    quint16 GetBatteryLevel();

    /// gets the current finger position
    /// oFingerPos is a pointer to a NUM_FINGER long array to write to
    /// each value is 0 - 100 where 100 is fully extended and 0 is fully closed
    void GetFingerPos(quint16* oFingerPos);
    
    
signals:
    void fingerPositionUpdated();
    void batteryLevelUpdated();
    
protected:
	
    void run();
    void SetFingerPos(quint16* iFingerPos);
    void SetBatteryLevel(quint16 iBatteryLevel);
    
    void SetPwmForFinger(int iValue, int iFingerNum);
    void SetDirForFinger(FingerDir iFingerDir, int iFingerNum);

    /// gets the currently used drive levels
    /// oDriveLevel is a pointer to a NUM_FINGER long array to write to
//    void GetFingerDriveLevel(int16_t* oDriveLevel);
//    void GetFingerDir(FingerDir* oFingerDir);
    void UpdatePwmControlStates();
    
	void ReadFingerPositions();
	void ReadBatteryLevel();
	
private:
	void closeFiles();

	bool m_done;

    /// file descriptors for each PWM output
    int pwmFileDescriptors[NUM_FINGERS];
    
    /// file descriptors for each GPIO output
    int gpioFileDescriptors[NUM_FINGERS];
       
    /// protects the PWM state data
    QMutex controlMutex;
    
    /// State of PWM Output
    enum PwmState
    {
        PWM_NORMAL,                     ///< pwm level can change, but not direction
        PRE_WAIT_TO_CHANGE_DIR,     ///< SetFingerDrive has rxed a change direction and pwm has been set to 0
        RXED_WAIT_TO_CHANGE_DIR,    ///< main loop has seen direction change, but not acted
        PRE_WAIT_TO_SET_PWR,        ///< main loop has waited at Pwm = 0, set GPIO on entry, now wait to set non-zero Pwm
        // POST_WAIT is not needed since it goes directly back to Normal
    };
    
    /// Current state for each finger's PWM & associated GPIO
    PwmState pwmState[NUM_FINGERS];
    
    /// protects the rest of the data
    QMutex dataMutex;

    /// output data to drive the GPIO lines
    FingerDir fingerDirs[NUM_FINGERS];
    
    /// pwm level for each finger (positive only, direction is contained in fingerDirs)
    qint16 fingerPwmLevel[NUM_FINGERS];
    
    /// current finger position samples
    quint16 currPositionSample[NUM_FINGERS];
    
    /// current battery level
    quint16 batteryLevel;
};

#endif
