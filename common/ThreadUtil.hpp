#pragma once
#include <sys/syscall.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <thread>

namespace Common { 
inline auto setThreadCore(int core_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);          // Clear the cpu_set_t variable
  CPU_SET(core_id, &cpuset);  // enable entry for the core_id
  return (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
          0);
}

template<typename T, typename... A>
inline auto createAndStartThread(int core_id, const std::string& name, T&& func,
                                 A&&... args) noexcept {
  std::atomic<bool> running(false), failed(false);
  auto thread_body = [&] {
    if (core_id >= 0 && !setThreadCore(core_id)) {
      std::cerr << "Failed to set core affinity for " << name << " "
                << pthread_self() << " to " << core_id << std::endl;

      failed = true;
      return;
    }
    std::cout << "Set core affinity for " << name << " " << pthread_self()
              << " to " << core_id << std::endl;
    running = true;
    std::forward<T>(func)((std::forward<A>(args))...);
  };
  auto t = new std::thread(thread_body);
  while (!running && !failed) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
  }
  if (failed) {
    t->join();
    delete t;
    t = nullptr;
  }
  return t;
}
}