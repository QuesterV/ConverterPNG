#-------------------------------------------------
#
# Project created by QtCreator 2015-11-18T14:21:09
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11 //поддержка с++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ConverterPNG
TEMPLATE = app


SOURCES +=  src/main.cpp\
            src/MainWindow.cpp \
            src/MultiConverter.cpp

HEADERS  += src/MainWindow.h \
            src/QueueTS.h \
            src/MultiConverter.h

FORMS    += src/MainWindow.ui

RESOURCES += res.qrc


