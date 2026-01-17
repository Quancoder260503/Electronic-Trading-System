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
LIB = pthread

# Core TCP library components
TCP_SRC = TCPServer.cc TCPSocket.cc
TCP_OBJ = $(TCP_SRC:.cc=.o)

# Example programs
EXAMPLES = SocketExample LoggingExample ThreadExample QueueExample
EXAMPLE_OBJ = SocketExample.o LoggingExample.o ThreadExample.o QueueExample.o
EXAMPLE_EXE = $(addprefix .dist/, $(EXAMPLES))

ALL_EXE = $(EXAMPLE_EXE)

.DEFAULT_GOAL := all

all: $(EXAMPLE_EXE)

# Socket example target
.dist/SocketExample: SocketExample.o TCPServer.o TCPSocket.o
	@mkdir -p .dist
	$(CXX) $(CXXFLAGS) -o $@ $^ -l$(LIB)

# Logging example target
.dist/LoggingExample: LoggingExample.o
	@mkdir -p .dist
	$(CXX) $(CXXFLAGS) -o $@ $^ -l$(LIB)

# Thread example target
.dist/ThreadExample: ThreadExample.o
	@mkdir -p .dist
	$(CXX) $(CXXFLAGS) -o $@ $^ -l$(LIB)

# Queue example target
.dist/QueueExample: QueueExample.o
	@mkdir -p .dist
	$(CXX) $(CXXFLAGS) -o $@ $^ -l$(LIB)

# Generic compilation rule
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Explicit dependencies
SocketExample.o : SocketExample.cc TCPServer.hpp TCPSocket.hpp Logging.hpp
TCPServer.o     : TCPServer.cc TCPServer.hpp TCPSocket.hpp Logging.hpp TimeUtil.hpp
TCPSocket.o     : TCPSocket.cc TCPSocket.hpp Logging.hpp SocketUtil.hpp TimeUtil.hpp
LoggingExample.o: LoggingExample.cc Logging.hpp
ThreadExample.o : ThreadExample.cc ThreadUtil.hpp
QueueExample.o  : QueueExample.cc ThreadUtil.hpp LockFreeQueue.hpp Mempool.hpp

.PHONY: format clean debug all run

format:
	clang-format -style=file -i *.cc *.hpp

debug:
	$(MAKE) all DEBUG=1

clean:
	rm -f *.o $(ALL_EXE)
	rm -rf .dist

run-socket: .dist/SocketExample
	./.dist/SocketExample

run-logging: .dist/LoggingExample
	./.dist/LoggingExample

run-thread: .dist/ThreadExample
	./.dist/ThreadExample

run-queue: .dist/QueueExample
	./.dist/QueueExample