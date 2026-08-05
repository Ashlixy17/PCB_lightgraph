QT += core gui testlib
CONFIG += console testcase c++11
TEMPLATE = app
TARGET = progressive_rendering_tests

INCLUDEPATH += ..

SOURCES += \
    tst_progressive_rendering.cpp \
    ../progressiverendercontroller.cpp \
    ../progressiverenderutils.cpp

HEADERS += \
    ../progressiverendercontroller.h \
    ../progressiverenderutils.h \
    ../ledstrip.h
