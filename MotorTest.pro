TEMPLATE = app

TARGET = MotorTest

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


INCLUDEPATH += .

HEADERS += handcontrolthread.h \
           motorspeeddlg.h \
           motortest.h

SOURCES += handcontrolthread.cpp \
           main.cpp \
           motorspeeddlg.cpp \
           motortest.cpp

FORMS += motortest.ui

target.path = /usr/bin
INSTALLS += target

