#include "MatchingEngine.hpp"

namespace Exchange {
MatchingEngine::MatchingEngine(ClientRequestLFQueue* client_request,
                               ClientResponseLFQueue* client_response,
                               MarketUpdateLFQueue* market_updates)
    : incoming_requests(client_request),
      outgoing_responses(client_response),
      outgoing_market_updates(market_updates),
      logger("exchange_matching_engine.log") {
  for (size_t i = 0; i < ticker_order_book.size(); i++) {
    ticker_order_book[i] = new MatchingEngineOrderBook(i, &logger, this);
  }
}
MatchingEngine::~MatchingEngine() {
  run = false;
  using namespace std::literals::chrono_literals;
  std::this_thread::sleep_for(1s);  // wait for all threads to finish up
  incoming_requests = nullptr;
  outgoing_market_updates = nullptr;
  outgoing_market_updates = nullptr;
  for (auto& order_book : ticker_order_book) {
    delete order_book;
    order_book = nullptr;
  }
}
auto MatchingEngine::start() -> void {
  run = true;
  ASSERT(Common::createAndStartThread(-1, "exchange/matching/MatchingEngine",
                                      [this]() { run(); }) != nullptr,
         "Faild to start Matching Engine thread.");
}
auto MatchingEngine::stop() -> void { run = false; }
}  // namespace Exchange