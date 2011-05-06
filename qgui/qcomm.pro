#-------------------------------------------------
#
# Project created by QtCreator 2011-05-05T23:35:43
#
#-------------------------------------------------

QT       += core gui

TARGET = qcomm
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    dialogconnect.cpp

HEADERS  += mainwindow.h \
    dialogconnect.h

FORMS    += mainwindow.ui \
    dialogconnect.ui

LIBS += ../../libcomm/libcomm.a
