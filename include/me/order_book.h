#pragma once
#include "me/types.h"
#include "order.h"
#include "trade.h"
#include <map>
#include <unordered_map>
#include <vector>
#include <list>
#include <optional>

namespace me {
    class OrderBook{
        private:
            struct OrderLocation {
                Side side; 
                Price price;
                std::list<Order>::iterator iter;
            };
            std::map<Price, std::list<Order>, std::greater<Price>>  bids_;
            std::map<Price, std::list<Order>, std::less<Price>>     asks_;
            std::unordered_map<OrderId, OrderLocation>              index_;
            
        public:
            std::vector<Trade> add_order(const Order& order);
            bool cancel_order(OrderId id);
            std::optional<Price> best_bid() const;
            std::optional<Price> best_ask() const;

    };
} // namespace me
