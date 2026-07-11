#pragma once
#include "me/order.h"
#include "me/trade.h"
#include "me/types.h"
#include <optional>
#include <vector>

namespace me {

// Deliberately simple oracle implementation — correctness over speed.
// Uses linear scans and sorted vectors. No shared code with OrderBook.
class ReferenceMatcher {
public:
    std::vector<Trade> add_order(const Order& order);
    bool cancel_order(OrderId id);
    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    std::size_t index_size() const { return bids_.size() + asks_.size(); }

private:
    std::vector<Order> bids_;  // all resting buy orders
    std::vector<Order> asks_;  // all resting sell orders
};

} // namespace me
