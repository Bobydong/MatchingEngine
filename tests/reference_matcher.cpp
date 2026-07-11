#include "reference_matcher.h"
#include <algorithm>

namespace me {

std::vector<Trade> ReferenceMatcher::add_order(const Order& order) {
    // Reject duplicates
    for (const Order& o : bids_) { if (o.id == order.id) { return {}; } }
    for (const Order& o : asks_) { if (o.id == order.id) { return {}; } }

    std::vector<Trade> trades;
    Order incoming = order;

    if (incoming.side == Side::Buy) {
        // Sort asks: lowest price first, then earliest timestamp (price-time priority)
        std::sort(asks_.begin(), asks_.end(), [](const Order& a, const Order& b) {
            return a.price != b.price ? a.price < b.price : a.timestamp < b.timestamp;
        });

        for (Order& resting : asks_) {
            if (incoming.quantity == 0) break;
            if (incoming.type == OrderType::Limit && resting.price > incoming.price) { break; }

            Quantity traded_qty = std::min(incoming.quantity, resting.quantity);
            trades.push_back({resting.id, incoming.id, resting.price, traded_qty, incoming.timestamp});
            incoming.quantity -= traded_qty;
            resting.quantity  -= traded_qty;
        }

        // Remove fully consumed asks
        asks_.erase(std::remove_if(asks_.begin(), asks_.end(),
            [](const Order& o) { return o.quantity == 0; }), asks_.end());

    } else {
        // Sort bids: highest price first, then earliest timestamp
        std::sort(bids_.begin(), bids_.end(), [](const Order& a, const Order& b) {
            return a.price != b.price ? a.price > b.price : a.timestamp < b.timestamp;
        });

        for (Order& resting : bids_) {
            if (incoming.quantity == 0) { break; }
            if (incoming.type == OrderType::Limit && resting.price < incoming.price) { break; }

            Quantity traded_qty = std::min(incoming.quantity, resting.quantity);
            trades.push_back({resting.id, incoming.id, resting.price, traded_qty, incoming.timestamp});
            incoming.quantity -= traded_qty;
            resting.quantity  -= traded_qty;
        }

        bids_.erase(std::remove_if(bids_.begin(), bids_.end(),
            [](const Order& o) { return o.quantity == 0; }), bids_.end());
    }

    // Rest unfilled limit remainder
    if (incoming.quantity > 0 && incoming.type == OrderType::Limit) {
        if (incoming.side == Side::Buy) { bids_.push_back(incoming); }
        else { asks_.push_back(incoming); }
    }

    return trades;
}

bool ReferenceMatcher::cancel_order(OrderId id) {
    for (auto it = bids_.begin(); it != bids_.end(); ++it) {
        if (it->id == id) { bids_.erase(it); return true; }
    }
    for (auto it = asks_.begin(); it != asks_.end(); ++it) {
        if (it->id == id) { asks_.erase(it); return true; }
    }
    return false;
}

std::optional<Price> ReferenceMatcher::best_bid() const {
    if (bids_.empty()) { return std::nullopt; }
    Price best = bids_[0].price;
    for (const Order& o : bids_) { best = std::max(best, o.price); }
    return best;
}

std::optional<Price> ReferenceMatcher::best_ask() const {
    if (asks_.empty()) { return std::nullopt; }
    Price best = asks_[0].price;
    for (const Order& o : asks_) { best = std::min(best, o.price); }
    return best;
}

} // namespace me
