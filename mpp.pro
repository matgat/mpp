TEMPLATE = app
CONFIG += c++11
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
#include(deployment.pri)
#qtcAddDeployment()

SOURCES += main.cpp \
    unt_Dictionary.cpp \
    unt_PoorMansUnicode.cpp \
    unt_MatUts.cpp


HEADERS += \
    unt_Dictionary.h \
    unt_PoorMansUnicode.h \
    unt_MatUts.h


# Enable args globbing in msvc (Windows)
msvc:OBJECTS += setargv.obj

