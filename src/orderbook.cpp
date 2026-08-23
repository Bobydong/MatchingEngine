#include "matching_engine/orderbook.h"
#include "matching_engine/types.h"
#include <algorithm>


bool ME::orderbook::add_order(const ME::Order& order) {
    // If order ID already exists, reject the order and return false
    if (order_locations_.find(order.order_id) != order_locations_.end()) {
        return false;
    }
    // Check if the order is a bid or an ask and add it to the appropriate map
    if (order.side == ME::Side::BID) {
        auto& level = bids_[order.price];  // Find the price level in the bids map
            // If the price level doesn't exist, it will automatically create a new list for it
        level.push_back(order);    // Add the order to the list at that price level
        // Store the location of the order in the order_locations_ map
        order_locations_[order.order_id] = {ME::Side::BID, order.price, std::prev(level.end())};
    }
    else if (order.side == ME::Side::ASK) {
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
    if (order.side == ME::Side::BID) {
        // Remove the order from the list of orders at that price level
        auto list_it = bids_.find(order.price);
        list_it->second.erase(order.iterator);
        // Check if list is now emtpy, if so remove the price level from the map
        if (list_it->second.empty()) {
            bids_.erase(list_it);
        }
    }
    else if (order.side == ME::Side::ASK) {
        auto list_it = asks_.find(order.price);
        list_it->second.erase(order.iterator);
        if (list_it->second.empty()) {
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

std::optional<ME::Fill> ME::orderbook::reduce_best_order_quantity(ME::Side side, ME::Quantity amount) {
    // A non-positive reduction is a caller bug. Reject it before touching anything 
    // A negative amount would otherwise INFLATE the resting order.
    if (amount <= 0) {
        return std::nullopt;
    }

    if (side == ME::Side::BID) {
        if (bids_.empty()) {
            return std::nullopt; // No best bid to reduce
        }
        auto level_it = bids_.begin();           // Best bid price level
        auto& best_level = level_it->second;
        auto& best_order = best_level.front();   // Oldest order at that price (FIFO)

        // Clamp to what is actually resting, and report back what we really took.
        const ME::Quantity filled = std::min(amount, best_order.quantity);
        const ME::Fill fill{best_order.maker_id, best_order.price, filled};
        best_order.quantity -= filled;

        if (best_order.quantity == 0) {
            // Read the id BEFORE pop_front() destroys the node. Reading through
            // best_order after the pop is a use-after-free.
            const ME::ID filled_id = best_order.order_id;
            best_level.pop_front();
            order_locations_.erase(filled_id);   // Keep the two containers in sync
            if (best_level.empty()) {
                bids_.erase(level_it);           // Drop the now-empty price level
            }
        }
        return fill;
    }
    else if (side == ME::Side::ASK) {
        if (asks_.empty()) {
            return std::nullopt;
        }
        auto level_it = asks_.begin();
        auto& best_level = level_it->second;
        auto& best_order = best_level.front();

        const ME::Quantity filled = std::min(amount, best_order.quantity);
        const ME::Fill fill{best_order.maker_id, best_order.price, filled};

        best_order.quantity -= filled;
        if (best_order.quantity == 0) {
            const ME::ID filled_id = best_order.order_id;
            best_level.pop_front();
            order_locations_.erase(filled_id);
            if (best_level.empty()) {
                asks_.erase(level_it);
            }
        }
        return fill;
    }
    return std::nullopt;
}