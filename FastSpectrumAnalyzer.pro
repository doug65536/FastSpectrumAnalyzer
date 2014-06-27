#-------------------------------------------------
#
# Project created by QtCreator 2014-06-25T18:09:54
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FastSpectrumAnalyzer
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    fft.cpp \
    asserts.cpp \
    stopwatch.cpp \
    tostring.cpp \
    voiceprintview.cpp \
    audiotest.cpp \
    taskqueue.cpp

HEADERS  += mainwindow.h \
    fft.h \
    asserts.h \
    stopwatch.h \
    tostring.h \
    voiceprintview.h \
    audiotest.h \
    taskqueue.h

FORMS    += mainwindow.ui

CONFIG += c++11
CONFIG += mobility
CONFIG += warn_on

MOBILITY = 

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

OTHER_FILES += \
    android/AndroidManifest.xml

