#include "matching_engine/orderbook.h"
#include "matching_engine/types.h"


bool ME::orderbook::add_order(const ME::Order& order) {
    // If order ID already exists, reject the order and return false
    if (order_locations_.find(order.order_id) != order_locations_.end()){
        return false;
    }
    // Check if the order is a bid or an ask and add it to the appropriate map
    if (order.side == ME::Side::BID){
        auto& level = bids_[order.price];  // Find the price level in the bids map
            // If the price level doesn't exist, it will automatically create a new list for it
        level.push_back(order);    // Add the order to the list at that price level
        // Store the location of the order in the order_locations_ map
        order_locations_[order.order_id] = {ME::Side::BID, order.price, std::prev(level.end())};
    }
    else if (order.side == ME::Side::ASK){
        auto& level = asks_[order.price];  
        level.push_back(order);   
        order_locations_[order.order_id] = {ME::Side::ASK, order.price, std::prev(level.end())};
    }
    return true;
}

bool ME::orderbook::cancel_order(const ME::ID& order_id) {
    auto it = order_locations_.find(order_id);
    // If the order is not found, return false
    if (it == order_locations_.end()) {
        return false;
    }

    const ME::OrderLocation& order = it->second;
    if (order.side == ME::Side::BID){
        // Remove the order from the list of orders at that price level
        auto list_it = bids_.find(order.price);
        list_it->second.erase(order.iterator);
        // Check if list is now emtpy, if so remove the price level from the map
        if (list_it->second.empty()){
            bids_.erase(list_it);
        }
    }
    else if (order.side == ME::Side::ASK){
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

bool ME::orderbook::reduce_best_order_quantity(const Side& side, const ME::Quantity& amount_to_reduce) {
    if (bids_.empty() && asks_.empty()) {
        return false; // No orders to reduce
    }
    if (side == Side::BID) {
        auto& best_level = bids_.begin()->second; // Get the list of orders at the best bid price
        auto& best_order = best_level.front(); // Get the first order in the list
        if (best_order.quantity < amount_to_reduce) {
            return false; // Not enough quantity to reduce
        }
        best_order.quantity -= amount_to_reduce; // Reduce the quantity
        if (best_order.quantity == 0) {
            best_level.pop_front(); // Remove the order if quantity is zero
        }
        if (best_level.empty()) {
            bids_.erase(bids_.begin()); // Remove the price level if no orders left
        }
    }
    else if (side == Side::ASK) {
        auto& best_level = asks_.begin()->second; 
        auto& best_order = best_level.front(); 
        if (best_order.quantity < amount_to_reduce) {
            return false; 
        }
        best_order.quantity -= amount_to_reduce; 
        if (best_order.quantity == 0) {
            best_level.pop_front(); 
        }
        if (best_level.empty()) {
            asks_.erase(asks_.begin()); 
        }
    }
    return true;
}

ME::Quantity ME::orderbook::get_best_order_quantity(const Side& side) const {
    if (side == Side::BID) {
        if (bids_.empty()){
            return 0;
        }
        return bids_.begin()->second.front().quantity; // Return the quantity of the first order at the best bid price
    }
    else if (side == Side::ASK) {
        if (asks_.empty()){
            return 0;
        }
        return asks_.begin()->second.front().quantity; 
    }
    return 0; // Should never reach here
}