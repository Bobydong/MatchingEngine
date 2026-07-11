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

    std::vector<Trade> trades;
    Order incoming = order;  

    if (incoming.side == Side::Buy) {
        // Buy order matches against asks (lowest ask first)
        while (incoming.quantity > 0 && !asks_.empty()) {
            auto best_it = asks_.begin();
            Price best_price = best_it->first;

            // Stop if incoming limit price is below the best ask
            if (incoming.type == OrderType::Limit && best_price > incoming.price) {
                break;
            }

            std::list<Order>& queue = best_it->second;
            while (incoming.quantity > 0 && !queue.empty()) {
                Order& resting = queue.front();
                Quantity traded_qty = std::min(incoming.quantity, resting.quantity);

                trades.push_back({resting.id, incoming.id, best_price, traded_qty, incoming.timestamp});

                incoming.quantity -= traded_qty;
                resting.quantity  -= traded_qty;

                if (resting.quantity == 0) {
                    index_.erase(resting.id);
                    queue.pop_front();
                }
            }

            // Remove empty price level so best_ask() stays honest
            if (queue.empty()) asks_.erase(best_it);
        }
    } 
    else {
        // Sell order matches against bids (highest bid first)
        while (incoming.quantity > 0 && !bids_.empty()) {
            auto best_it = bids_.begin();
            Price best_price = best_it->first;

            // Stop if incoming limit price is above the best bid
            if (incoming.type == OrderType::Limit && best_price < incoming.price) break;

            std::list<Order>& queue = best_it->second;
            while (incoming.quantity > 0 && !queue.empty()) {
                Order& resting = queue.front();
                Quantity traded_qty = std::min(incoming.quantity, resting.quantity);

                trades.push_back({resting.id, incoming.id, best_price, traded_qty, incoming.timestamp});

                incoming.quantity -= traded_qty;
                resting.quantity  -= traded_qty;

                if (resting.quantity == 0) {
                    index_.erase(resting.id);
                    queue.pop_front();
                }
            }

            if (queue.empty()) bids_.erase(best_it);
        }
    }

    // Rest any unfilled remainder as a limit order
    if (incoming.quantity > 0 && incoming.type == OrderType::Limit) {
        if (incoming.side == Side::Buy) {
            std::list<Order>& queue = bids_[incoming.price];
            queue.push_back(incoming);
            index_[incoming.id] = {incoming.side, incoming.price, std::prev(queue.end())};
        } else {
            std::list<Order>& queue = asks_[incoming.price];
            queue.push_back(incoming);
            index_[incoming.id] = {incoming.side, incoming.price, std::prev(queue.end())};
        }
    }

    return trades;
}