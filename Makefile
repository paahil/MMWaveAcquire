CXXFLAGS := -std=c++17 -DASIO_STANDALONE -DWIN32

EXE:=datacard.exe
BASEDIR = $(CURDIR)

ASIOINCLUDE := $(BASEDIR)/libs/asio/include


all: $(EXE)

$(EXE): datacardcontrol.cpp
	g++ $(CXXFLAGS) datacardcontrol.cpp -o $(EXE) -I$(ASIOINCLUDE) -lws2_32

test: test.exe

test.exe: udptest.cpp
	g++ $(CXXFLAGS) udptest.cpp -o test.exe -I$(ASIOINCLUDE) -lws2_32

format: format.exe

format.exe: formattest.cpp
	g++ $(CXXFLAGS) formattest.cpp -o format.exe -lws2_32
