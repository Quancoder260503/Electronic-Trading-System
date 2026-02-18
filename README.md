# Electronic Trading System

A high-performance, C++20 electronic trading simulation stack with separate **exchange** and **trading client** processes.

The project includes:
- low-level shared infrastructure (`common/`) for sockets, logging, threading, memory pools, and lock-free queues,
- an exchange-side matching and market-data pipeline (`exchange/`),
- and trading-side gateway, market data consumption, and strategy engines (`trading/`).

## Repository layout

```text
.
├── common/                  # Shared low-latency utilities and primitives
│   ├── LockFreeQueue.hpp
│   ├── Mempool.hpp
│   ├── Logging.hpp
│   ├── TCPServer/TCPSocket/McastSocket
│   └── ThreadUtil.hpp, TimeUtil.hpp, Types.hpp
├── exchange/                # Exchange process
│   ├── exchange_main.cc
│   ├── matching/            # Order/OrderBook/MatchingEngine
│   ├── order_server/        # Client request-response over TCP
│   └── market_data/         # Snapshot + incremental market data publishing
├── trading/                 # Trading process
│   ├── trading_main.cc
│   ├── order_gateway/       # Exchange client connectivity
│   ├── market_data/         # Market data receiver
│   └── strategy/            # TradeEngine, risk, order mgmt, strategies
├── Makefile                 # Top-level build and run orchestration
└── test_socket_example.sh   # Legacy helper script (currently references old target)
```

## Architecture overview

### Exchange side
- `matching/MatchingEngine` processes client order flow and updates books.
- `order_server/OrderServer` accepts client TCP requests and returns responses.
- `market_data/MarketDataPublisher` emits snapshot and incremental market updates.

### Trading side
- `order_gateway/Gateway` sends client requests and receives order responses.
- `market_data/MarketDataConsumer` subscribes to multicast updates.
- `strategy/TradeEngine` coordinates strategy logic, risk checks, and order management.

### Shared primitives
- lock-free queues and memory pooling for low-latency message passing,
- thread helpers and logging abstractions,
- typed domain models (`ClientID`, `OrderID`, `TickerID`, `Price`, `Quantity`, `AlgoType`, etc.).

## Prerequisites

- Linux (or compatible POSIX environment)
- `g++` with C++20 support
- `make`
- `clang-format` (optional, for formatting target)

## Build

Build both exchange and trading binaries:

```bash
make
```

Build individual binaries:

```bash
make exchange
make trading
```

Build outputs are written to:

```text
.dist/exchange_main
.dist/trading_main
```

## Run

### 1) Start exchange

```bash
make run-exchange
```

Or use the combined launcher:

```bash
make run all TRADING_ARGS="1 RANDOM 10 0.50 100 500 5.0"
```

Set `DETAILS=1` to print recent generated log files after the run:

```bash
make run all TRADING_ARGS="1 RANDOM 10 0.50 100 500 5.0" DETAILS=1
```

### 2) Start a trading client

`trading_main` expects:

```text
<client_id> <algo_type> [per-ticker config blocks...]
```

Each ticker config block has 5 values:

```text
<clip> <threshold> <risk_max_order_size> <risk_max_position> <risk_max_loss>
```

Example for one ticker using `RANDOM` strategy:

```bash
make run-trading TRADING_ARGS="1 RANDOM 10 0.50 100 500 5.0"
```

Supported algorithm names are defined in `common/Types.hpp` (`RANDOM`, `MAKER`, `TAKER`).

## Useful make targets

```text
make all         # Build exchange_main and trading_main
make exchange    # Build only exchange_main
make trading     # Build only trading_main
make run all     # Run exchange + trading together (requires TRADING_ARGS)
make debug       # Debug build flags
make sanitize    # AddressSanitizer + UndefinedBehaviorSanitizer build
make strict      # Strict warnings (includes -Wconversion)
make format      # clang-format across common/, exchange/, trading/
make clean       # Remove build artifacts
make help        # Show target help
```

Default builds use a clean warning profile. To enable stricter warning checks explicitly:

```bash
make WARN_PROFILE=strict
```

## Operational notes

- Current defaults use loopback and multicast addresses from source (`lo`, `233.252.x.x`, ports in `exchange_main.cc` and `trading_main.cc`).
- Logs are emitted per component (`exchange_main.log`, `trading_main_<client>.log`, etc.).
- Run exchange first, then one or more trading processes.

## Development guidance

- Keep changes focused and performance-aware.
- Prefer updating or adding clear run examples when changing behavior.
- If you alter message contracts (`ClientRequest`, `ClientResponse`, `MarketUpdate`), update both exchange and trading sides consistently.