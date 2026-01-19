#include <csignal> 

#include "matching/MatchingEngine.hpp"

Common::Logger *logger = nullptr; 
Exchange::MatchingEngine* matching_engine = nullptr; 

void signal_handler(int) { 
 using namespace std::literals::chrono_literals; 
 std::this_thread::sleep_for(10s); 

 delete logger; 
 logger = nullptr; 
 delete matching_engine; 
 matching_engine = nullptr; 

 std::this_thread::sleep_for(10s); 
 exit(EXIT_SUCCESS); 
}

int main(void) {
 logger = new Common::Logger("exchange_main.log"); 
 std::signal(SIGINT, signal_handler); 
 
 const int sleep_time = 100 * 1000; 
 
 Exchange::ClientRequestLFQueue  client_request(MATCHING_ENGINE_MAX_CLIENT_UPDATES); 
 Exchange::ClientResponseLFQueue client_response(MATCHING_ENGINE_MAX_CLIENT_UPDATES); 
 Exchange::MarketUpdateLFQueue   market_update(MATCHING_ENGINE_MAX_MARKET_UPDATES); 

 std::string time_str; 

 logger->log("%:% %() % Starting Matching Engine...\n", 
    __FILE__, __LINE__, __FUNCTION__, 
    Common::getCurrentTimeStr(&time_str));

 matching_engine = new Exchange::MatchingEngine(&client_request, &client_response, &market_update); 
 matching_engine->start(); 

 while(true) { 
    logger->log("%:% %() % Sleeping for a few milliseconds..\n", 
        __FILE__, __LINE__, __FUNCTION__, 
        Common::getCurrentTimeStr(&time_str));
    usleep(sleep_time * 1000); 
 }
 return 0; 
}