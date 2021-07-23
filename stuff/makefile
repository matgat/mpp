PRJNAME = mpp
SRCDIR = ../source
BLDDIR = ../build
VPATH := $(SRCDIR)
MAIN = $(SRCDIR)/main.cpp
HEADERS = $(wildcard $(SRCDIR)/*.hpp)

CXX = g++
CXXFLAGS = -std=c++2b -funsigned-char -Wextra -Wall -pedantic -O3 -DFMT_HEADER_ONLY -Isource/fmt/include
#CXXFLAGS = /std:c++latest /utf-8 /J /W4 /O2

ifeq ($(OS),Windows_NT)
    TARGET = $(BLDDIR)/$(PRJNAME).exe
else
    TARGET = $(BLDDIR)/$(PRJNAME)
endif


all: executable

debug: CXXFLAGS += -DDEBUG -D_DEBUG -g
debug: executable

executable: $(MAIN) $(HEADERS) makefile
	$(CXX) -o $(TARGET) $(CXXFLAGS) $(MAIN)

clean:
	rm *.o
