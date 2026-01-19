#include <iostream>

#include "LockFreeQueue.hpp"
#include "ThreadUtil.hpp"

struct MyStruct {
  int d[3];
};

using namespace Common;

auto ConsumeFunction(LockFreeQueue<MyStruct>* lock_free_queue) {
  using namespace std::literals::chrono_literals;
  std::this_thread::sleep_for(5s);
  while (lock_free_queue->size()) {
    const auto elem = lock_free_queue->getNextToRead();
    lock_free_queue->updateReadIndex();
    std::cout << "consumeFunction read elem : " << elem->d[0] << " "
              << elem->d[1] << " " << elem->d[2]
              << " lfq-size : " << lock_free_queue->size() << std::endl;
    // std::flush(std::cout);
    std::this_thread::sleep_for(5s);
  }
  std::cout << "consumeFunction exiting. " << std::endl;
}

int main(void) {
  LockFreeQueue<MyStruct> lock_free_queue(20);
  auto ct = createAndStartThread(-1, "", ConsumeFunction, &lock_free_queue);
  for (int i = 0; i < 50; i++) {
    const MyStruct curr{i, i + 1, i + 2};
    (*lock_free_queue.getNextToWrite()) = curr;
    lock_free_queue.updateWriteIndex();
    std::cout << "main constructed element : " << curr.d[0] << " " << curr.d[1]
              << " " << curr.d[2] << " lfq-size : " << lock_free_queue.size()
              << std::endl;
    // std::flush(std::cout);
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(1s);
  }
  ct->join();
  std::cout << "main exiting ." << std::endl;
  return 0;
}