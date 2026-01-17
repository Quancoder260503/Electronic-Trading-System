CXX = g++
CXXSTD = -std=c++17
CXXWARN = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wcast-align -Werror
OPT = -O2
DEBUG ?= 0
SAN = 
DEBUG_FLAGS =
ifeq ($(DEBUG),1)
SAN = -fsanitize=address,undefined -fno-omit-frame-pointer -g
DEBUG_FLAGS = -DDEBUG_INVALID_REQUESTS
endif
CXXFLAGS = $(CXXSTD) $(OPT) $(CXXWARN) $(SAN) $(DEBUG_FLAGS)
LIB = 
CPPSRC = SocketExample.cc TCPServer.cc TCPSocket.cc
OBJ = $(CPPSRC:.cc=.o)

.DEFAULT_GOAL := all

all: clean $(EXE)

$(EXE): $(OBJ) 
	$(CXX) $(CXXFLAGS) -o $(EXE) $(OBJ) $(LIB)

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $

SocketExample.o : SocketExample.cc TCPServer.hpp TCPSocket.hpp
TCPServer.o     : TCPServer.cc TCPSocket.hpp
TCPSocket.o     : TCPSocket.cc Logging.hpp SocketUtil.hpp

.PHONY: format clean debug all

format:
	clang-format -style=file -i *.cc *.hpp

debug:
	$(MAKE) all DEBUG=1

clean:
	rm -f $(OBJ) $(EXE)