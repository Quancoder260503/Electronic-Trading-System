SHELL := /bin/bash

CXX ?= g++
DEBUG ?= 0
SANITIZE ?= 0
WARN_PROFILE ?= clean
DETAILS ?= 0

BUILD_DIR ?= .dist
OBJ_DIR ?= $(BUILD_DIR)/obj

CXXSTD := -std=c++20
BASE_WARNINGS := -Wall -Wextra -Wpedantic
STRICT_WARNINGS := $(BASE_WARNINGS) -Wshadow -Wconversion -Wcast-align
CLEAN_WARNINGS := $(BASE_WARNINGS) -Wshadow -Wcast-align -Wno-unused-parameter -Wno-switch

ifeq ($(WARN_PROFILE),strict)
	WARNINGS := $(STRICT_WARNINGS)
else
	WARNINGS := $(CLEAN_WARNINGS)
endif

OPT_FLAGS := -O2
DBG_FLAGS := -g3 -DDEBUG_INVALID_REQUESTS
SAN_FLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer

CPPFLAGS := -I. -Icommon -Iexchange -Itrading
CXXFLAGS := $(CXXSTD) $(WARNINGS) $(OPT_FLAGS) -MMD -MP
LDFLAGS :=
LDLIBS := -pthread

ifeq ($(DEBUG),1)
  CXXFLAGS := $(filter-out $(OPT_FLAGS),$(CXXFLAGS)) $(DBG_FLAGS)
endif

ifeq ($(SANITIZE),1)
  CXXFLAGS += $(SAN_FLAGS)
  LDFLAGS += $(SAN_FLAGS)
endif

rwildcard = $(foreach d,$(wildcard $(1)/*),$(call rwildcard,$(d),$(2))) $(filter $(subst *,%,$(2)),$(wildcard $(1)/$(2)))

COMMON_SRCS := $(call rwildcard,common,*.cc)
EXCHANGE_SRCS := $(call rwildcard,exchange,*.cc)
TRADING_SRCS := $(call rwildcard,trading,*.cc)

EXCHANGE_MAIN_SRC := exchange/exchange_main.cc
TRADING_MAIN_SRC := trading/trading_main.cc

EXCHANGE_LIB_SRCS := $(filter-out $(EXCHANGE_MAIN_SRC),$(EXCHANGE_SRCS))
TRADING_LIB_SRCS := $(filter-out $(TRADING_MAIN_SRC),$(TRADING_SRCS))

BIN_EXCHANGE := $(BUILD_DIR)/exchange_main
BIN_TRADING := $(BUILD_DIR)/trading_main

EXCHANGE_OBJS := $(patsubst %.cc,$(OBJ_DIR)/%.o,$(COMMON_SRCS) $(EXCHANGE_LIB_SRCS) $(EXCHANGE_MAIN_SRC))
TRADING_OBJS := $(patsubst %.cc,$(OBJ_DIR)/%.o,$(COMMON_SRCS) $(TRADING_LIB_SRCS) $(TRADING_MAIN_SRC))

DEPFILES := $(sort $(EXCHANGE_OBJS:.o=.d) $(TRADING_OBJS:.o=.d))

.DEFAULT_GOAL := all

.PHONY: all exchange trading run run-all run-exchange run-trading debug sanitize strict clean format help

all: exchange trading

exchange: $(BIN_EXCHANGE)

trading: $(BIN_TRADING)

run:
	@if [[ "$(filter exchange,$(MAKECMDGOALS))" == "exchange" ]]; then \
		$(MAKE) --no-print-directory run-exchange; \
	elif [[ "$(filter trading,$(MAKECMDGOALS))" == "trading" ]]; then \
		$(MAKE) --no-print-directory run-trading TRADING_ARGS="$(TRADING_ARGS)"; \
	elif [[ "$(filter all,$(MAKECMDGOALS))" == "all" ]]; then \
		$(MAKE) --no-print-directory run-all TRADING_ARGS="$(TRADING_ARGS)" DETAILS="$(DETAILS)"; \
	else \
		echo "Specify what to run: make run exchange | make run trading TRADING_ARGS='...' | make run all TRADING_ARGS='...' [DETAILS=1]"; \
		exit 1; \
	fi

run-all: $(BIN_EXCHANGE) $(BIN_TRADING)
	@if [[ -z "$(TRADING_ARGS)" ]]; then \
		echo "TRADING_ARGS is required."; \
		echo "Example: make run all TRADING_ARGS='1 RANDOM 10 0.5 100 200 0.7'"; \
		exit 1; \
	fi
	@set -euo pipefail; \
	echo "Starting exchange..."; \
	./$(BIN_EXCHANGE) & \
	ex_pid=$$!; \
	cleanup() { \
		if kill -0 $$ex_pid 2>/dev/null; then \
			echo "Stopping exchange (pid $$ex_pid)..."; \
			kill $$ex_pid; \
			wait $$ex_pid 2>/dev/null || true; \
		fi; \
	}; \
	trap cleanup EXIT INT TERM; \
	sleep 1; \
	echo "Starting trading with args: $(TRADING_ARGS)"; \
	./$(BIN_TRADING) $(TRADING_ARGS); \
	if [[ "$(DETAILS)" == "1" ]]; then \
		echo ""; \
		echo "Recent log files:"; \
		ls -1t *.log 2>/dev/null | head -n 12 || true; \
	fi

$(BIN_EXCHANGE): $(EXCHANGE_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN_TRADING): $(TRADING_OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: %.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run-exchange: $(BIN_EXCHANGE)
	./$(BIN_EXCHANGE)

run-trading: $(BIN_TRADING)
	@if [[ -z "$(TRADING_ARGS)" ]]; then \
		echo "TRADING_ARGS is required."; \
		echo "Example: make run-trading TRADING_ARGS='1 RANDOM 10 0.5 100 200 0.7'"; \
		exit 1; \
	fi
	./$(BIN_TRADING) $(TRADING_ARGS)

debug:
	$(MAKE) all DEBUG=1 SANITIZE=0

sanitize:
	$(MAKE) all DEBUG=1 SANITIZE=1

strict:
	$(MAKE) all WARN_PROFILE=strict

format:
	find common exchange trading -type f \( -name '*.hpp' -o -name '*.cc' \) -print0 | xargs -0 clang-format -i

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  all           Build exchange_main and trading_main (default)"
	@echo "  exchange      Build only exchange_main"
	@echo "  trading       Build only trading_main"
	@echo "  run           Run one target: make run exchange | make run trading TRADING_ARGS='...' | make run all TRADING_ARGS='...' [DETAILS=1]"
	@echo "  run-all       Build and run exchange + trading together (requires TRADING_ARGS)"
	@echo "  run-exchange  Build and run exchange_main"
	@echo "  run-trading   Build and run trading_main (requires TRADING_ARGS)"
	@echo "  debug         Build with debug symbols"
	@echo "  sanitize      Build with ASan + UBSan"
	@echo "  strict        Build with strict warnings (includes -Wconversion)"
	@echo "  format        Run clang-format over source and headers"
	@echo "  clean         Remove all build artifacts"

-include $(DEPFILES)