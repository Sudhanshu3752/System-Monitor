CXX = g++
CXXFLAGS = -std=c++17 -Wall
LDFLAGS = -lncurses

SRC = src/main.cpp src/system_info.cpp
INC = -Iinclude

all:
	$(CXX) $(SRC) $(INC) $(LDFLAGS) -o system_monitor

run: all
	./system_monitor

clean:
	rm -f system_monitor
