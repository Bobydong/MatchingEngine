#pragma once
#include "order_book.h"
#include <optional>
#include <vector>


namespace me {
    class MatchingEngine {
        private:
            OrderBook book_;
        public:
            std::vector<Trade> add_order(const Order& order);
            bool cancel_order(OrderId id);
            std::optional<Price> best_bid() const;
            std::optional<Price> best_ask() const;
    };
} // namespace me
