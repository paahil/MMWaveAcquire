CXXFLAGS := -std=c++11 -DWIN32

EXE:=datacard.exe
BASEDIR := $(CURDIR)
SRCDIR :=  $(BASEDIR)/src
TESTDIR := $(BASEDIR)/test




all: $(EXE)

$(EXE): $(SRCDIR)/datacardcontrol.cpp
	g++ $(CXXFLAGS) $< -o $@  -lws2_32

test: test.exe

test.exe: $(TESTDIR)/udptest.cpp
	g++ $(CXXFLAGS)  $< -o $@ -lws2_32

format: format.exe

format.exe: $(TESTDIR)/formattest.cpp
	g++ $(CXXFLAGS)  $< -o $@ -lws2_32
