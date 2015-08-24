TEMPLATE = app
CONFIG += c++11
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    unt_Dictionary.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    unt_Dictionary.h

