#include "matching_engine/orderbook.h"


std::vector<ME::Trade> ME::orderbook::add_order(const ME::Order& order) {
    // If order ID already exists, return empty vector of trades
    
}

            // struct OrderLocation{
            //     Side side;
            //     Price price;
            //     std::list<Order>::iterator iterator; // 
            // };
            // std::map<Price, std::list<Order>, std::greater<Price>> bids_;  
            // std::map<Price, std::list<Order>> asks_;
            // std::unordered_map<OrderId, OrderLocation> order_locations_;

bool ME::orderbook::cancel_order(const ME::OrderId& id) {
    auto it = order_locations_.find(id);
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