#include <csignal> 

#include "matching/MatchingEngine.hpp"
#include "market_data/MarketDataPublisher.hpp"
#include "order_server/OrderServer.hpp"

Common::Logger *logger = nullptr; 
Exchange::MatchingEngine* matching_engine = nullptr; 
Exchange::MarketDataPublisher* market_data_publisher = nullptr;
Exchange::OrderServer *order_server = nullptr; 

void signal_handler(int) { 
 using namespace std::literals::chrono_literals; 
 std::this_thread::sleep_for(10s); 

 delete logger; 
 logger = nullptr; 
 delete matching_engine; 
 matching_engine = nullptr; 
 delete market_data_publisher; 
 market_data_publisher = nullptr; 
 delete order_server; 
 order_server = nullptr; 
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
 
 const std::string mkt_pub_iface = "lo"; 
 const std::string snap_pub_ip   = "233.252.14.1"; 
 const std::string incr_pub_ip   = "233.252.14.5"; 
 const int snap_pub_port = 20000; 
 const int inc_pub_port  = 20002; 

 logger->log("%:% %() % Starting Market Data Publisher...\n", 
   __FILE__, __LINE__, __FUNCTION__, 
   Common::getCurrentTimeStr(&time_str)
 );
 market_data_publisher = new Exchange::MarketDataPublisher(&market_update, mkt_pub_iface, snap_pub_ip, snap_pub_port, incr_pub_ip, inc_pub_port);
 market_data_publisher->start();

 const std::string order_gw_iface = "lo";
 const int order_gw_port = 12345;

  logger->log("%:% %() % Starting Order Server...\n", 
   __FILE__, __LINE__, __FUNCTION__, 
   Common::getCurrentTimeStr(&time_str)
  );
  order_server = new Exchange::OrderServer(&client_request, &client_response, order_gw_iface, order_gw_port);
  order_server->start();

  while(true) { 
    logger->log("%:% %() % Sleeping for a few milliseconds..\n", 
      __FILE__, __LINE__, __FUNCTION__, 
      Common::getCurrentTimeStr(&time_str)
    );
    usleep(sleep_time * 1000); 
  }
  return 0; 
}