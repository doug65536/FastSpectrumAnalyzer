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
    taskqueue.cpp \
    inputtoolbox.cpp

HEADERS  += mainwindow.h \
    fft.h \
    asserts.h \
    stopwatch.h \
    tostring.h \
    voiceprintview.h \
    audiotest.h \
    taskqueue.h \
    inputtoolbox.h

FORMS    += mainwindow.ui \
    inputtoolbox.ui

CONFIG += c++11
CONFIG += mobility
CONFIG += warn_on
CONFIG += threads
CONFIG += ltcg
CONFIG += rtti

MOBILITY = 

QMAKE_CXXFLAGS_DEBUG -= -O0
QMAKE_LFLAGS_DEBUG += -O0

QMAKE_CXXFLAGS_DEBUG += -march=native
QMAKE_CXXFLAGS_RELEASE += -march=native
QMAKE_LFLAGS_RELEASE += -march=native

QMAKE_CXXFLAGS_RELEASE += -ffast-math -g -ftree-vectorize
QMAKE_LFLAGS_RELEASE += -ffast-math -g -ftree-vectorize
#QMAKE_CXXFLAGS_RELEASE += -pg
#QMAKE_LFLAGS_RELEASE += -pg
QMAKE_CXXFLAGS_RELEASE += -O3
QMAKE_LFLAGS_RELEASE += -O3
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_LFLAGS_RELEASE -= -O2

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android

OTHER_FILES += \
    android/AndroidManifest.xml

