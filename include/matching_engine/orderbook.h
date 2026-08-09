#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <optional>
#include <vector>
#include "types.h"

namespace ME {
    class orderbook {
        private:
            std::map<Price, std::list<Order>, std::greater<Price>> bids_;  
            std::map<Price, std::list<Order>> asks_;
            std::unordered_map<ID, OrderLocation> order_locations_;

        public:
            bool add_order(const Order& order);
            bool cancel_order(const ID& order_id);
            
            std::optional<Price> best_bid() const;
            std::optional<Price> best_ask() const;

            std::size_t numberOfOrders() const { return order_locations_.size(); }
    };
}
