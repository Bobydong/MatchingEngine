#include "matching_engine/orderbook.h"
#include "matching_engine/types.h"


bool ME::orderbook::add_order(const ME::Order& order) {
    // If order ID already exists, reject the order and return false
    if (order_locations_.find(order.order_id) != order_locations_.end()){
        return false;
    }
    // Check if the order is a bid or an ask and add it to the appropriate map
    if (order.side == ME::Side::BID){
        auto it = bids_.find(order.price);  // Find the price level in the bids map
        if (it == bids_.end()){ // If the price level doesn't exist, create a new list for it
            bids_[order.price] = std::list<ME::Order>();
        }
        bids_[order.price].push_back(order);    // Add the order to the list at that price level
        // Store the location of the order in the order_locations_ map
        order_locations_[order.order_id] = {ME::Side::BID, order.price, std::prev(bids_[order.price].end())};
    }
    if (order.side == ME::Side::ASK){
        auto it = asks_.find(order.price);
        if (it == asks_.end()){
            asks_[order.price] = std::list<ME::Order>();
        }
        asks_[order.price].push_back(order);
        order_locations_[order.order_id] = {ME::Side::ASK, order.price, std::prev(asks_[order.price].end())};
    }
    return true;
}

bool ME::orderbook::cancel_order(const ME::ID& order_id) {
    auto it = order_locations_.find(order_id);
    // If the order is not found, return false
    if (it == order_locations_.end()) {
        return false;
    }

    ME::OrderLocation& order = it->second;
    if (order.side == ME::Side::BID){
        // Remove the order from the list of orders at that price level
        auto list_it = bids_.find(order.price);
        list_it->second.erase(order.iterator);
        // Check if list is now emtpy, if so remove the price level from the map
        if (list_it->second.empty()){
            bids_.erase(list_it);
        }
    }
    if (order.side == ME::Side::ASK){
        auto list_it = asks_.find(order.price);
        list_it->second.erase(order.iterator);
        if (list_it->second.empty()){
            asks_.erase(list_it);
        }
    }
    order_locations_.erase(it);
    return true;
}

std::optional<ME::Price> ME::orderbook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<ME::Price> ME::orderbook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}