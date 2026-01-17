#include "ThreadUtil.hpp" 
#include <iostream> 

auto dummyFunction(int a, int b, bool sleep) { 
 std::cout << "dummy Function (" << a << " " << b << ")" << std::endl; 
 std::cout << "dummy Function output " << a + b << std::endl; 
 if(sleep) { 
   std::cout << "dummy Function sleeping..." << std::endl; 
   using namespace std::literals::chrono_literals; 
   std::this_thread::sleep_for(5s); 
 }
 std::cout << "dummy Function done." << std::endl; 
 return; 
} 


int main(void) {
 //using namespace Common; 
 auto thread_1 = createAndStartThread(-1, "dummyFunction1", dummyFunction, 12, 21, false); 
 auto thread_2 = createAndStartThread(1 , "dummyFunction2", dummyFunction, 15, 51, true); 
 auto thread_3 = createAndStartThread(2 , "dummyFunction3", dummyFunction, 17, 71, false); 
 std::cout << "main waiting for threads to be done. " << std::endl; 
 thread_1->join(); 
 thread_2->join(); 
 std::cout <<  "main exiting. " << std::endl; 
 return 0; 
}