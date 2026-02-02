CXX = g++
CXXSTD = -std=c++20
CXXWARN = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wcast-align 
OPT = -O2
DEBUG ?= 0
SAN = 
DEBUG_FLAGS =
ifeq ($(DEBUG),1)
SAN = -fsanitize=address,undefined -fno-omit-frame-pointer -g
DEBUG_FLAGS = -DDEBUG_INVALID_REQUESTS
endif
CXXFLAGS = $(CXXSTD) $(OPT) $(CXXWARN) $(SAN) $(DEBUG_FLAGS)
INCLUDES = -I. -Icommon -Iexchange
LIB = pthread

# Directories
COMMON_DIR = common
EXCHANGE_DIR = exchange
MATCHING_DIR = $(EXCHANGE_DIR)/matching
ORDER_SERVER_DIR = $(EXCHANGE_DIR)/order_server
MARKET_DATA_DIR = $(EXCHANGE_DIR)/market_data

# Common TCP library components
COMMON_TCP_SRC = $(COMMON_DIR)/TCPServer.cc $(COMMON_DIR)/TCPSocket.cc
COMMON_TCP_OBJ = $(COMMON_TCP_SRC:.cc=.o)

# Common networking components
COMMON_NET_SRC = $(COMMON_DIR)/McastSocket.cc
COMMON_NET_OBJ = $(COMMON_NET_SRC:.cc=.o)

# Exchange matching engine components
MATCHING_SRC = $(MATCHING_DIR)/Order.cc $(MATCHING_DIR)/MatchingEngine.cc $(MATCHING_DIR)/OrderBook.cc
MATCHING_OBJ = $(MATCHING_SRC:.cc=.o)

# Exchange order server components
ORDER_SERVER_SRC = $(ORDER_SERVER_DIR)/OrderServer.cc
ORDER_SERVER_OBJ = $(ORDER_SERVER_SRC:.cc=.o)
ORDER_SERVER_HEADERS = $(ORDER_SERVER_DIR)/ClientRequest.hpp $(ORDER_SERVER_DIR)/ClientResponse.hpp $(ORDER_SERVER_DIR)/OrderServer.hpp $(ORDER_SERVER_DIR)/FifoSequencer.hpp

# Exchange market data components
MARKET_DATA_SRC = $(MARKET_DATA_DIR)/MarketDataPublisher.cc $(MARKET_DATA_DIR)/SnapshotSynthesizer.cc
MARKET_DATA_OBJ = $(MARKET_DATA_SRC:.cc=.o)
MARKET_DATA_HEADERS = $(MARKET_DATA_DIR)/MarketUpdate.hpp $(MARKET_DATA_DIR)/MarketDataPublisher.hpp $(MARKET_DATA_DIR)/SnapshotSynthesizer.hpp

# Exchange main program
EXCHANGE_MAIN = exchange_main
EXCHANGE_MAIN_SRC = $(EXCHANGE_DIR)/$(EXCHANGE_MAIN).cc
EXCHANGE_MAIN_EXE = .dist/$(EXCHANGE_MAIN) 

ALL_EXE = $(EXAMPLE_EXE) $(EXCHANGE_MAIN_EXE)

.DEFAULT_GOAL := all

all: $(EXAMPLE_EXE) $(EXCHANGE_MAIN_EXE)

# Socket example target
.dist/SocketExample: SocketExample.o $(COMMON_TCP_OBJ)
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

# Exchange main target
$(EXCHANGE_MAIN_EXE): $(EXCHANGE_MAIN_SRC:.cc=.o) $(COMMON_TCP_OBJ) $(COMMON_NET_OBJ) $(MATCHING_OBJ) $(ORDER_SERVER_OBJ) $(MARKET_DATA_OBJ)
	@mkdir -p .dist
	$(CXX) $(CXXFLAGS) -o $@ $^ -l$(LIB)

# Generic compilation rule
%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Explicit dependencies
SocketExample.o              : SocketExample.cc $(COMMON_DIR)/TCPServer.hpp $(COMMON_DIR)/TCPSocket.hpp $(COMMON_DIR)/Logging.hpp
$(COMMON_DIR)/TCPServer.o    : $(COMMON_DIR)/TCPServer.cc $(COMMON_DIR)/TCPServer.hpp $(COMMON_DIR)/TCPSocket.hpp $(COMMON_DIR)/Logging.hpp $(COMMON_DIR)/TimeUtil.hpp
$(COMMON_DIR)/TCPSocket.o    : $(COMMON_DIR)/TCPSocket.cc $(COMMON_DIR)/TCPSocket.hpp $(COMMON_DIR)/Logging.hpp $(COMMON_DIR)/SocketUtil.hpp $(COMMON_DIR)/TimeUtil.hpp
LoggingExample.o             : LoggingExample.cc $(COMMON_DIR)/Logging.hpp
ThreadExample.o              : ThreadExample.cc $(COMMON_DIR)/ThreadUtil.hpp
QueueExample.o               : QueueExample.cc $(COMMON_DIR)/ThreadUtil.hpp $(COMMON_DIR)/LockFreeQueue.hpp $(COMMON_DIR)/Mempool.hpp
$(MATCHING_DIR)/Order.o      : $(MATCHING_DIR)/Order.cc $(MATCHING_DIR)/Order.hpp $(COMMON_DIR)/Types.hpp $(COMMON_DIR)/Logging.hpp
$(MATCHING_DIR)/OrderBook.o  : $(MATCHING_DIR)/OrderBook.cc $(MATCHING_DIR)/OrderBook.hpp $(MATCHING_DIR)/Order.hpp $(COMMON_DIR)/Types.hpp $(COMMON_DIR)/Logging.hpp
$(MATCHING_DIR)/MatchingEngine.o : $(MATCHING_DIR)/MatchingEngine.cc $(MATCHING_DIR)/MatchingEngine.hpp $(MATCHING_DIR)/OrderBook.hpp $(MATCHING_DIR)/Order.hpp $(COMMON_DIR)/Types.hpp $(COMMON_DIR)/Logging.hpp
$(ORDER_SERVER_DIR)/ClientRequest.hpp : $(COMMON_DIR)/Types.hpp $(COMMON_DIR)/LockFreeQueue.hpp
$(ORDER_SERVER_DIR)/ClientResponse.hpp : $(COMMON_DIR)/Types.hpp $(COMMON_DIR)/LockFreeQueue.hpp
$(COMMON_DIR)/McastSocket.o : $(COMMON_DIR)/McastSocket.cc $(COMMON_DIR)/McastSocket.hpp $(COMMON_DIR)/SocketUtil.hpp $(COMMON_DIR)/Logging.hpp
$(MARKET_DATA_DIR)/MarketUpdate.hpp : $(COMMON_DIR)/Types.hpp $(COMMON_DIR)/LockFreeQueue.hpp
$(MARKET_DATA_DIR)/MarketDataPublisher.o : $(MARKET_DATA_DIR)/MarketDataPublisher.cc $(MARKET_DATA_DIR)/MarketDataPublisher.hpp $(MARKET_DATA_DIR)/MarketUpdate.hpp $(COMMON_DIR)/McastSocket.hpp $(COMMON_DIR)/Logging.hpp
$(MARKET_DATA_DIR)/SnapshotSynthesizer.o : $(MARKET_DATA_DIR)/SnapshotSynthesizer.cc $(MARKET_DATA_DIR)/SnapshotSynthesizer.hpp $(MARKET_DATA_DIR)/MarketUpdate.hpp $(COMMON_DIR)/Logging.hpp
$(ORDER_SERVER_DIR)/OrderServer.o : $(ORDER_SERVER_DIR)/OrderServer.cc $(ORDER_SERVER_DIR)/OrderServer.hpp $(ORDER_SERVER_DIR)/FifoSequencer.hpp $(ORDER_SERVER_DIR)/ClientRequest.hpp $(ORDER_SERVER_DIR)/ClientResponse.hpp $(COMMON_DIR)/TCPServer.hpp $(COMMON_DIR)/Logging.hpp
$(EXCHANGE_DIR)/$(EXCHANGE_MAIN).o : $(EXCHANGE_MAIN_SRC) $(MATCHING_DIR)/MatchingEngine.hpp $(MATCHING_DIR)/OrderBook.hpp $(COMMON_DIR)/TCPServer.hpp

.PHONY: format clean debug all run run-socket run-logging run-thread run-queue run-exchange help

format:
	clang-format -style=file -i *.cc *.hpp
	clang-format -style=file -i $(COMMON_DIR)/*.cc $(COMMON_DIR)/*.hpp
	clang-format -style=file -i $(EXCHANGE_DIR)/*/*.cc $(EXCHANGE_DIR)/*/*.hpp

debug:
	$(MAKE) all DEBUG=1

clean:
	rm -f *.o $(COMMON_TCP_OBJ) $(COMMON_NET_OBJ) $(MATCHING_OBJ) $(ORDER_SERVER_OBJ) $(MARKET_DATA_OBJ) $(ALL_EXE)
	rm -rf .dist

run-socket: .dist/SocketExample
	./.dist/SocketExample

run-logging: .dist/LoggingExample
	./.dist/LoggingExample

run-thread: .dist/ThreadExample
	./.dist/ThreadExample

run-queue: .dist/QueueExample
	./.dist/QueueExample

run-exchange: $(EXCHANGE_MAIN_EXE)
	./$(EXCHANGE_MAIN_EXE)

help:
	@echo "Available targets:"
	@echo "  all              - Build all example programs and exchange_main (default)"
	@echo "  debug            - Build with debug flags and sanitizers"
	@echo "  run-socket       - Build and run SocketExample"
	@echo "  run-logging      - Build and run LoggingExample"
	@echo "  run-thread       - Build and run ThreadExample"
	@echo "  run-queue        - Build and run QueueExample"
	@echo "  run-exchange     - Build and run exchange_main"
	@echo "  format           - Format all source files with clang-format"
	@echo "  clean            - Remove all build artifacts"
	@echo "  help             - Show this help message"