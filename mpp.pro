TEMPLATE = app
CONFIG += c++11
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
#include(deployment.pri)
#qtcAddDeployment()

message(Building $$TARGET)

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

#message($$QMAKESPEC)
win32-g++ {
    message(Remove MinGW libstdc++-6.dll dependency)
    # -static -fno-rtti -fno-exceptions
    QMAKE_LFLAGS += -static-libgcc -static-libstdc++
    QMAKE_LFLAGS += -fno-rtti
}
