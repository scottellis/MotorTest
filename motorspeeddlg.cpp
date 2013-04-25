/*
 * Copyright (c) 2013 Neurolutions, Inc.
 *
 */

#include <qboxlayout.h>
#include "motorspeeddlg.h"

MotorSpeedDlg::MotorSpeedDlg(int speed, QWidget *parent)
	: QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint)
{
	layoutWindow();
	setWindowTitle("Motor Speed");

	connect(m_speedSlider, SIGNAL(valueChanged(int)), SLOT(speedChanged(int)));
	connect(m_okBtn, SIGNAL(clicked()), SLOT(accept()));
	connect(m_cancelBtn, SIGNAL(clicked()), SLOT(reject()));

	m_speedSlider->setSliderPosition(speed);
}

int MotorSpeedDlg::speed()
{
	if (m_speedSlider)
		return m_speedSlider->sliderPosition();

	return 0;
}

void MotorSpeedDlg::speedChanged(int speed)
{
	m_speedLbl->setText(QString::number(speed));
}

void MotorSpeedDlg::layoutWindow()
{
	setFixedWidth(280);

	QVBoxLayout *vLayout = new QVBoxLayout(this);

	m_speedLbl = new QLabel("0");
	m_speedLbl->setFixedWidth(40);
	m_speedLbl->setFrameShadow(QFrame::Sunken);
	m_speedLbl->setFrameShape(QFrame::Panel);

	m_speedSlider = new QSlider;
	m_speedSlider->setRange(0, 100);
	m_speedSlider->setTickInterval(10);
	m_speedSlider->setSingleStep(2);
	m_speedSlider->setTickPosition(QSlider::TicksBelow);
	m_speedSlider->setOrientation(Qt::Horizontal);

	QHBoxLayout *hLayout = new QHBoxLayout;	
	hLayout->addWidget(m_speedLbl);
	hLayout->addWidget(m_speedSlider, 1);
	vLayout->addLayout(hLayout);

	vLayout->addSpacerItem(new QSpacerItem(10, 30, QSizePolicy::Fixed, QSizePolicy::Fixed));

	m_okBtn = new QPushButton("OK");
	m_okBtn->setDefault(true);
	m_cancelBtn = new QPushButton("Cancel");

	hLayout = new QHBoxLayout;
	hLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Fixed));
	hLayout->addWidget(m_okBtn);
	hLayout->addWidget(m_cancelBtn);
	vLayout->addLayout(hLayout);
}