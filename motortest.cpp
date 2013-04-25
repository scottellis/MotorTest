/*
 * Copyright (c) 2013 Neurolutions, Inc.
 *
 */

#include <qboxlayout.h>
#include <qgroupbox.h>
#include <qformlayout.h>

#include "motortest.h"
#include "motorspeeddlg.h"

MotorTest::MotorTest(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	layoutWindow();
	initControls();

	setWindowFlags(Qt::CustomizeWindowHint);

	m_runSpeed = 70;
	m_running = false;
	m_newBatteryData = false;
	m_newFingerPosData = false;

	connect(m_actionExit, SIGNAL(clicked()), SLOT(close()));
	connect(m_actionStart, SIGNAL(clicked()), SLOT(onStart()));
	connect(m_actionStop, SIGNAL(clicked()), SLOT(onStop()));
	connect(m_actionSpeed, SIGNAL(clicked()), SLOT(onSpeed()));

	m_handThread = new HandControlThread();

	connect(m_handThread, SIGNAL(fingerPositionUpdated()), SLOT(fingerPositionUpdated()));
	connect(m_handThread, SIGNAL(batteryLevelUpdated()), SLOT(batteryLevelUpdated()));

	m_handThread->startThread();

	m_timer = startTimer(100);
}

MotorTest::~MotorTest()
{
}

void MotorTest::closeEvent(QCloseEvent *)
{
	killTimer(m_timer);

	m_handThread->stopThread();
}

void MotorTest::timerEvent(QTimerEvent *)
{
	quint16 data[2];

	if (m_newBatteryData) {
		data[0] = m_handThread->GetBatteryLevel();
		m_batteryLevelLbl->setText(QString::number(data[0]));
	}

	if (m_newFingerPosData) { 
		m_handThread->GetFingerPos(data);
		m_positionLbl[0]->setText(QString::number(data[0]));
		m_positionLbl[1]->setText(QString::number(data[1]));
	}
}

void MotorTest::batteryLevelUpdated()
{
	QMutexLocker lock(&m_newBatteryDataMutex);
	m_newBatteryData = true;
}

void MotorTest::fingerPositionUpdated()
{
	QMutexLocker lock(&m_newFingerPosDataMutex);
	m_newFingerPosData = true;
}

void MotorTest::onStart()
{
	qint16 speed[2];

	m_directionBtn[0]->setEnabled(false);
	m_directionBtn[1]->setEnabled(false);
	m_applyDirectionBtn->setEnabled(false);
	m_actionStart->setEnabled(false);
	m_actionSpeed->setEnabled(false);

	if (m_directionBtn[0]->isChecked())
		speed[0] = -m_runSpeed;
	else
		speed[0] = m_runSpeed;

	speed[1] = speed[0];

	m_handThread->SetFingerDrive(speed);
	m_running = true;
	m_runStatusLbl->setText("Running");
}

void MotorTest::onStop()
{
	qint16 speed[2];

	m_directionBtn[0]->setEnabled(true);
	m_directionBtn[1]->setEnabled(true);
	m_actionStart->setEnabled(true);
	m_actionSpeed->setEnabled(true);

	speed[0] = 0;
	speed[1] = 0;
	m_handThread->SetFingerDrive(speed);

	m_running = false;
	m_runStatusLbl->setText("Stopped");
}

void MotorTest::onSpeed()
{
	MotorSpeedDlg dlg(m_runSpeed, this);

	if (dlg.exec() == QDialog::Accepted)
		m_runSpeed = dlg.speed();
}

void MotorTest::onDirectionChange()
{
	m_applyDirectionBtn->setEnabled(true);
	m_actionStart->setEnabled(false);
	m_actionSpeed->setEnabled(false);
}

void MotorTest::onApplyDirection()
{
	m_applyDirectionBtn->setEnabled(false);
	m_actionStart->setEnabled(true);
	m_actionSpeed->setEnabled(true);
}

void MotorTest::initControls()
{
	connect(m_directionBtn[0], SIGNAL(clicked()), SLOT(onDirectionChange()));
	connect(m_directionBtn[1], SIGNAL(clicked()), SLOT(onDirectionChange()));
	connect(m_applyDirectionBtn, SIGNAL(clicked()), SLOT(onApplyDirection()));
	m_applyDirectionBtn->setEnabled(false);
}

void MotorTest::layoutWindow()
{
	setMaximumSize(320, 240);

	QVBoxLayout *vLayout = new QVBoxLayout;

	QHBoxLayout *hLayout = new QHBoxLayout;
	m_actionExit = new QPushButton("Exit");
	hLayout->addWidget(m_actionExit);
	m_actionStart = new QPushButton("Start");
	hLayout->addWidget(m_actionStart);
	m_actionStop = new QPushButton("Stop");
	hLayout->addWidget(m_actionStop);
	m_actionSpeed = new QPushButton("Speed...");
	hLayout->addWidget(m_actionSpeed);
	hLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Fixed));
	vLayout->addLayout(hLayout);

	hLayout = new QHBoxLayout;
	m_directionBtn[0] = new QRadioButton("Open");
	m_directionBtn[0]->setChecked(true);
	hLayout->addWidget(m_directionBtn[0]);
	m_directionBtn[1] = new QRadioButton("Close");
	hLayout->addWidget(m_directionBtn[1]);

	m_applyDirectionBtn = new QPushButton("Apply");
	hLayout->addWidget(m_applyDirectionBtn);

	hLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Fixed));
	QGroupBox *group = new QGroupBox("Direction");
	group->setLayout(hLayout);
	vLayout->addWidget(group);
	
	hLayout = new QHBoxLayout;

	for (int i = 0; i < 2; i++) {
		hLayout->addWidget(new QLabel(QString("Motor %1").arg(i+1)));

		m_positionLbl[i] = new QLabel("0");
		m_positionLbl[i]->setFixedWidth(60);
		m_positionLbl[i]->setFrameShadow(QFrame::Sunken);
		m_positionLbl[i]->setFrameShape(QFrame::Panel);
		hLayout->addWidget(m_positionLbl[i]);
	}
	hLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Fixed));
	vLayout->addLayout(hLayout);

	vLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Fixed, QSizePolicy::Expanding));
	
	centralWidget()->setLayout(vLayout);

	m_statusBar = new QStatusBar;
	setStatusBar(m_statusBar);

	m_runStatusLbl = new QLabel("Stopped");
	m_runStatusLbl->setFrameShadow(QFrame::Sunken);
	m_runStatusLbl->setFrameShape(QFrame::Panel);
	m_statusBar->addWidget(m_runStatusLbl, 1);

	m_batteryLevelLbl = new QLabel("Battery: 0");
	m_batteryLevelLbl->setFrameShadow(QFrame::Sunken);
	m_batteryLevelLbl->setFrameShape(QFrame::Panel);
	m_statusBar->addWidget(m_batteryLevelLbl);

}
