#-------------------------------------------------
#
# Project created by QtCreator 2017-10-31T15:15:58
#
#-------------------------------------------------

QT       += core
QT       += gui
QT       += websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TControllerRPi
TEMPLATE = app


SOURCES += main.cpp \
    mcp4725.cpp
SOURCES += tcontrollerrpi.cpp
SOURCES += clientlistdialog.cpp
SOURCES += utility.cpp

HEADERS += tcontrollerrpi.h \
    mcp4725.h
HEADERS += clientlistdialog.h
HEADERS += utility.h

FORMS    += tcontrollerrpi.ui
