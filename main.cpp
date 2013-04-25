/*
 * Copyright (c) 2013 Neurolutions, Inc.
 *
 */

#include "motortest.h"
#include <qapplication.h>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MotorTest w;
	w.show();

// only works with Qt4
#ifdef Q_WS_QWS
	w.showFullScreen();
#endif

	return a.exec();
}
