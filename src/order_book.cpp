#include "me/order_book.h"
#include "me/order.h"
#include "me/types.h"
#include <vector>

std::optional<me::Price> me::OrderBook::best_bid() const {
    if (bids_.empty()) {
       return std::nullopt;
    }
    return bids_.begin()->first;
}
std::optional<me::Price> me::OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

std::vector<me::Trade> me::OrderBook::add_order(const me::Order& order) {
    if (index_.contains(order.id)) {
        return {};
    }

    if (order.side == Side::Buy) {
        std::list<Order>& queue = bids_[order.price];
        queue.push_back(order);
        index_[order.id] = {order.side, order.price, std::prev(queue.end())};
    } else {
        std::list<Order>& queue = asks_[order.price];
        queue.push_back(order);
        index_[order.id] = {order.side, order.price, std::prev(queue.end())};
    }

    return {};
}