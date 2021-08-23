TEMPLATE = app
CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt

message(Building $$TARGET)

INCLUDEPATH += ../source/
VPATH += ../source/

SOURCES += main.cpp

HEADERS += \
    def-parser.hpp \
    dictionary.hpp \
    h-parser.hpp \
    logging.hpp \
    poor-mans-unicode.hpp \
    preprocess.hpp \
    string-utilities.hpp \
    system.hpp

DEFINES -= UNICODE
win32 {
    DEFINES += WIN32_LEAN_AND_MEAN
}


win32-g++ {
    message(g++ $$QMAKESPEC)
    QMAKE_CXXFLAGS += -O3 -Wextra -Wall -pedantic -funsigned-char
    # Removing MinGW libstdc++-6.dll dependency and other dlls
    QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++
    QMAKE_LFLAGS += -fno-rtti

} else:win32-clang-msvc {
    message(clang $$QMAKESPEC)
    QMAKE_CXXFLAGS += -Weverything -Wno-c++98-compat -Wno-missing-prototypes

} else:win32-msvc* {
    message(msvc $$QMAKESPEC)
    QMAKE_CXXFLAGS += /utf-8 /J
    #/std:c++latest /W4 /O2
    # Enable args globbing in msvc
    #OBJECTS += setargv.obj
}
