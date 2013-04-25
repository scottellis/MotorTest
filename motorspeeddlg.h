/*
 * Copyright (c) 2013 Neurolutions, Inc.
 *
 */

#ifndef MOTORSPEEDDLG_H
#define MOTORSPEEDDLG_H

#include <qdialog.h>
#include <qslider.h>
#include <qlabel.h>
#include <qpushbutton.h>

class MotorSpeedDlg : public QDialog
{
	Q_OBJECT

public:
	MotorSpeedDlg(int speed, QWidget *parent = 0);

	int speed();

public slots:
	void speedChanged(int);

private:
	void layoutWindow();
	void initControls();

	QLabel *m_speedLbl;
	QSlider *m_speedSlider;
	QPushButton *m_okBtn;
	QPushButton *m_cancelBtn;
};

#endif // MOTORSPEEDDLG_H
