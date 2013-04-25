/*
 * Copyright (c) 2013 Neurolutions, Inc.
 *
 */

#ifndef MOTORTEST_H
#define MOTORTEST_H

#include <qmainwindow.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qstatusbar.h>
#include <qmutex.h>

#include "ui_motortest.h"
#include "handcontrolthread.h"

class MotorTest : public QMainWindow
{
	Q_OBJECT

public:
	MotorTest(QWidget *parent = 0);
	~MotorTest();

public slots:
	void onStart();
	void onStop();
	void onSpeed();
	void onDirectionChange();
	void onApplyDirection();

	void batteryLevelUpdated();
	void fingerPositionUpdated();

protected:
	void closeEvent(QCloseEvent *);
	void timerEvent(QTimerEvent *);

private:
	void layoutWindow();
	void initControls();

	Ui::MotorTestClass ui;

	int m_timer;
	int m_runSpeed;
	bool m_running;
	QMutex m_newBatteryDataMutex;
	bool m_newBatteryData;
	QMutex m_newFingerPosDataMutex;
	bool m_newFingerPosData;
	
	HandControlThread *m_handThread;

	QRadioButton *m_directionBtn[2];
	QPushButton *m_applyDirectionBtn;
	QLineEdit *m_speedEdit;
	QLabel *m_positionLbl[2];
	QStatusBar *m_statusBar;
	QLabel *m_runStatusLbl;
	QLabel *m_batteryLevelLbl;
};

#endif // MOTORTEST_H
