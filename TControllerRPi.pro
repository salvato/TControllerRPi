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

CONFIG += c++11


SOURCES += main.cpp
SOURCES += mcp4725.cpp
SOURCES += ads1115.cpp
SOURCES += tcontrollerrpi.cpp
SOURCES += clientlistdialog.cpp
SOURCES += utility.cpp

HEADERS += tcontrollerrpi.h
HEADERS += mcp4725.h
HEADERS += ads1115.h
HEADERS += clientlistdialog.h
HEADERS += utility.h

FORMS    += tcontrollerrpi.ui
