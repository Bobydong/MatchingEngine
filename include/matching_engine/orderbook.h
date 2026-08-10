#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <optional>
#include "types.h"
#include <functional>
#include <cstddef>

namespace ME {
    // A pure data structure. The orderbook stores resting orders and maintains price/time priority. 
    // It does not match, does not generate trades, and does not validate orders.
    // 
    // The matching engine owns all of that: it receives an order, walks this book
    // to decide the fills, generates the Trades, and only then rests whatever
    // quantity is left over via add_order().
    // 
    // Preconditions on add_order() -- the caller (exchange or matching engine) is
    // responsible for these; the book will happily rest garbage:
    //   - order.quantity > 0
    //   - order.price is a valid tick
    //   - order.type == OrderType::LIMIT   (a MARKET order never rests)
    //   - order.symbol is this book's symbol

    class orderbook {
        private:
            std::map<Price, std::list<Order>, std::greater<Price>> bids_;  
            std::map<Price, std::list<Order>> asks_;
            std::unordered_map<ID, OrderLocation> order_locations_;

        public:
            // Rests an order. Returns false if order_id already exists.
            bool add_order(const Order& order);

            // Removes a resting order. Returns false if the id doesn't exist.
            bool cancel_order(const ID& order_id);

            // Returns the best bid/ask price, or std::nullopt if that side is empty.
            std::optional<Price> best_bid() const;
            std::optional<Price> best_ask() const;
            // Returns the number of resting orders in the book.
            std::size_t number_of_orders() const { return order_locations_.size(); }


            // --- Methods for Matching Engine --- 
            // Reduces the quantity of the best order on the given side by the specified amount.
            // Returns true if the best order was found and reduced, false if there was no best order or if the amount to reduce was greater than the best order's quantity.
            bool reduce_best_order_quantity(const Side& side, const Quantity& amount_to_reduce);
            // Returns the quantity of the best order on the given side, or 0 if there is no best order.
            Quantity get_best_order_quantity(const Side& side) const;
            
    };
}
