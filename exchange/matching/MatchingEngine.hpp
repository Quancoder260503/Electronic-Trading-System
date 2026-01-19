#pragma once
#include "OrderBook.hpp"
#include "common/LockFreeQueue.hpp"
#include "common/Macros.hpp"
#include "common/ThreadUtil.hpp"
#include "market_data/MarketUpdate.hpp"
#include "order_server/ClientRequest.hpp"
#include "order_server/ClientResponse.hpp"

namespace Exchange {
class MatchingEngine final {
 public:
  MatchingEngine(ClientRequestLFQueue *client_requests, ClientResponseLFQueue *client_responses,
                 MarketUpdateLFQueue *market_updates);
  ~MatchingEngine();
  auto start() -> void;
  auto stop() -> void;

  MatchingEngine() = delete;
  MatchingEngine(const MatchingEngine &) = delete;
  MatchingEngine(const MatchingEngine &&) = delete;
  MatchingEngine &operator=(const MatchingEngine &) = delete;
  MatchingEngine &operator=(const MatchingEngine &&) = delete;

  auto processClientRequest(const MatchingEngineClientRequest *client_request) noexcept -> void {
    auto order_book = ticker_order_book[client_request->ticker_id];
    switch (client_request->type) {
      case ClientRequestType::NEW: {
        order_book->add(client_request->client_id, client_request->ticker_id, client_request->side,
                        client_request->price, client_request->quantity);
      } break;

      case ClientRequestType::CANCEL {
        order_book->cancel(client_request->client_id;
                           client_request->order_id, client_request->ticker_id);
      } break;

          default: {
        FATAL("Received invalid client-request-type:" +
              ClientRequestTypeToString(client_request->type))
      } break;
    }
  }

  auto sendClientResponse(const MatchingEngineClientResponse *client_response) noexcept -> void {
    logger.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__,
               Common::getCurrentTimeStr(&time_str), client_response->toString());
    auto next_write = outgoing_responses->getNextToWrite();
    (*next_write) = std::move(*client_response);
    outgoing_responses->updateWriteIndex();
  }

  auto sendMarketUpdate(const MatchingEngineMarketUpdate *market_updates) noexcept -> void {
    logger.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__,
               Common::getCurrentTimeStr(&time_str), market_updates->toString());
    auto next_write = outgoing_market_updates->getNextToWrite();
    (*next_write) = *market_updates;
    outgoing_market_updates->updateWriteIndex();
  }

  auto run() noexcept -> void {
    logger.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__,
               Common::getCurrentTimeStr(&time_str));
    while (run) {
      const auto client_request = incoming_requests->getNextToRead();
      if (LIKELY(client_request)) {
        logger.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__,
                   Common::getCurrentTimeStr(&time_str), client_request->toString());
        processClientRequest(client_request);
        incoming_requests->updateReadIndex();
      }
    }
  }

 private:
  MatchingEngineOrderBook ticker_order_book;
  ClientRequestLFQueue *incoming_requests = nullptr;
  ClientResponseLFQueue *outgoing_responses = nullptr;
  MarketUpdateLFQueue *outgoing_market_updates = nullptr;

  volatile bool run = false;  // accessed by different threads
  std::string time_str;
  Logger logger;
}
}  // namespace Exchange