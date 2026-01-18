#pragma once 
#include <string> 
#include <fstream> 
#include <cstdio> 
#include "common/Macros.hpp"
#include "common/LockFreeQueue.hpp"
#include "common/ThreadUtil.hpp"
#include "common/TimeUtil.hpp"

namespace Common { 
  constexpr size_t LOCK_FREE_QUEUE_SIZE = 8 * 1024 * 1024; 
  enum class LogType : int8_t {
    CHAR = 0,
    INTEGER = 1, 
    LONG_INTEGER = 2, 
    LONG_LONG_INTEGER = 3,
    UNSIGNED_INTEGER = 4, 
    UNSIGNED_LONG_INTEGER = 5,
    UNSIGNED_LONG_LONG_INTEGER = 6,
    FLOAT = 7, 
    DOUBLE = 8
 };
 struct LogElement {
  LogType type = LogType::CHAR;
  union {
   char c;
   int i; 
   long l; 
   long long ll;
   unsigned u; 
   unsigned long ul; 
   unsigned long long ull;
   float f; 
   double d;
  } u_;
 };
 class Logger final { 
   public : 
     auto flushQueue() noexcept { 
        while(running) { 
          for(auto next = queue.getNextToRead(); queue.size() && next; next = queue.getNextToRead()) { 
            switch (next->type) {
              case LogType::CHAR                       : file << next->u_.c;   break; 
              case LogType::INTEGER                    : file << next->u_.i;   break; 
              case LogType::LONG_INTEGER               : file << next->u_.l;   break; 
              case LogType::LONG_LONG_INTEGER          : file << next->u_.ll;  break; 
              case LogType::UNSIGNED_INTEGER           : file << next->u_.u;   break; 
              case LogType::UNSIGNED_LONG_INTEGER      : file << next->u_.ul;  break; 
              case LogType::UNSIGNED_LONG_LONG_INTEGER : file << next->u_.ull; break; 
              case LogType::FLOAT                      : file << next->u_.f  ; break; 
              case LogType::DOUBLE                     : file << next->u_.d  ; break; 
            }
            queue.updateReadIndex(); 
          }
          file.flush(); 
          using namespace std::literals::chrono_literals; 
          std::this_thread::sleep_for(1ms); 
        }
     }

     explicit Logger(const std::string& file_name_) : 
        file_name(file_name_), queue(LOCK_FREE_QUEUE_SIZE) { 
       file.open(file_name); 
       ASSERT(file.is_open(), "Could not open the log file " + file_name); 
       logger_thread = createAndStartThread(-1, "Common/Logger", [this]() { 
         flushQueue(); 
       }); 
       ASSERT(logger_thread != nullptr, "Fail to start the Logger thread"); 
     }

     ~Logger() { 
       std::cerr << "Flushing and closing Logger for " << file_name << std::endl; 
       while(queue.size()) { 
        using namespace std::literals::chrono_literals; 
        std::this_thread::sleep_for(1s); 
       }
       running = false; 
       logger_thread->join(); 
       file.close(); 
     }

     Logger() = delete; 
     Logger(const Logger &) = delete; 
     Logger(const Logger &&) = delete; 
     Logger &operator = (const Logger &) = delete; 
     Logger &operator = (const Logger &&) = delete; 

     auto pushValue(const LogElement &log_element) noexcept { 
       *(queue.getNextToWrite()) = log_element; 
       queue.updateWriteIndex();  
     }

     auto pushValue(const char value) noexcept { 
       pushValue(LogElement{LogType::CHAR, {.c = value}}); 
     } 
     auto pushValue(const char *value) noexcept { 
       while(*value) { 
        pushValue(*value); 
        value++; 
       }
     }
     auto pushValue(const std::string &value) noexcept { 
       pushValue(value.c_str()); 
     }

     auto pushValue(const int value) noexcept { 
       pushValue(LogElement{LogType::INTEGER, {.i = value}});  
     }
     auto pushValue(const long value) noexcept { 
       pushValue(LogElement{LogType::LONG_INTEGER, {.l = value}});  
     }
     auto pushValue(const long long value) noexcept { 
       pushValue(LogElement{LogType::LONG_LONG_INTEGER, {.ll = value}});  
     }
     auto pushValue(const unsigned int value) noexcept { 
       pushValue(LogElement{LogType::UNSIGNED_INTEGER, {.u = value}});  
     }
     auto pushValue(const unsigned long int value) noexcept { 
       pushValue(LogElement{LogType::UNSIGNED_LONG_INTEGER, {.ul = value}});  
     }
     auto pushValue(const unsigned long long value) noexcept { 
       pushValue(LogElement{LogType::UNSIGNED_LONG_LONG_INTEGER, {.ull = value}});  
     }
     auto pushValue(const float value) noexcept { 
       pushValue(LogElement{LogType::FLOAT, {.f = value}});  
     }
     auto pushValue(const double value) noexcept { 
       pushValue(LogElement{LogType::DOUBLE, {.d = value}});  
     }

     template<typename T, typename ... A> 
     auto log(const char *s, const T& value, A ... args) noexcept { 
       while(*s) { 
        if(*s == '%') { 
          if(UNLIKELY(*(s + 1) == '%')) { 
            ++s; 
          }  else { 
            pushValue(value); 
            log(s + 1, args ...); 
            return; 
          }
        }
        pushValue(*s++); 
       }
       FATAL("extra arguments provided to log()"); 
     }

     auto log(const char *s) noexcept { 
        while(*s) { 
          if(*s == '%') { 
           if(UNLIKELY(*(s + 1) == '%')) { 
             ++s; 
           } else { 
            FATAL("missing arguments to log()"); 
           }
          }
          pushValue(*s++); 
        }
     }

    private: 
     const std::string file_name; 
     std::ofstream file; 
     LockFreeQueue<LogElement> queue; 
     std::atomic<bool> running = {true}; 
     std::thread *logger_thread = nullptr;  
 }; 
}