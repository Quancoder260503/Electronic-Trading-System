<!-- Copilot/AI instructions for contributors and coding agents -->
# Repository-specific instructions for AI coding agents

Purpose: help an AI quickly become productive in this C++ utility/examples repo.

- **Big picture:** this repository contains small, header-first utilities and examples for building an electronic-trading style toolkit: lock-free containers, a simple mempool, logging utilities, socket helpers, and threading helpers. Behavior and usage are demonstrated in the `*Example.cc` files at the repo root.

- **Key files to inspect (order matters):**
  - `LockFreeQueue.hpp` — lock-free queue implementation and usage constraints.
  - `Mempool.hpp` — custom memory pool used by lock-free structures.
  - `Logging.hpp` and `Macros.hpp` — logging API and macro conventions used across examples.
  - `SocketUtil.hpp` and `SocketExample.cc` — socket helper functions and sample usage.
  - `ThreadUtil.hpp` and `ThreadExample.cc` — thread helpers and patterns (RAII, join semantics).
  - Example drivers: `LoggingExample.cc`, `QueueExample.cc`, `SocketExample.cc`, `ThreadExample.cc` — sample programs that exercise APIs.
  - `README.md` — one-line project title; examples are the primary documentation.

- **Architecture & patterns:**
  - Header-first design: most core functionality is in `.hpp` files and intended for direct inclusion.
  - Examples are the canonical usage — prefer to update or add an example when changing behavior.
  - Low-level concurrency primitives are implemented (lock-free queues + mempool). Respect the memory/reclamation patterns used in `Mempool.hpp` when modifying `LockFreeQueue.hpp`.
  - Lightweight logging abstraction: updates should preserve the simple API in `Logging.hpp` and reuse macros from `Macros.hpp`.

- **Build / run (discovered):**
  - There is no top-level build system. Compile example programs directly. Typical command:

```bash
g++ -std=c++17 -O2 -pthread -I. LoggingExample.cc -o LoggingExample
g++ -std=c++17 -O2 -pthread -I. QueueExample.cc -o QueueExample
g++ -std=c++17 -O2 -pthread -I. SocketExample.cc -o SocketExample
g++ -std=c++17 -O2 -pthread -I. ThreadExample.cc -o ThreadExample
```

  - Run resulting binaries from the repo root. If adding new examples, follow the same single-file compilation pattern.

- **Local development conventions for AI edits:**
  - Do not change public header APIs without updating at least one example to show the new API.
  - Prefer minimal, focused changes. Preserve header-only style and inline implementations unless converting to a library is explicitly requested.
  - For concurrency or memory changes, add an example demonstrating correctness or a simple smoke test.

- **Search targets for understanding behavior:**
  - Grep for usage of macros and logging macros in `*Example.cc` files to learn how logging is used.
  - Inspect the top of each `.hpp` for usage notes and constraints (e.g., thread-safety, ownership rules).

- **Integration / external deps:**
  - The project appears self-contained and uses only standard C++ and POSIX sockets/threads. Avoid adding heavy external dependencies unless a clear reason exists.

- **When merging or refactoring:**
  - Keep the examples compiling with the `g++` command line above.
  - If adding a build system (CMake/Make), update README with build steps and preserve the single-file example compile commands as quick checks.

If any of these items are unclear or you'd like me to expand examples, add a lightweight CMakeLists.txt, or implement a specific change, tell me which area to prioritize and I will iterate.
