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

            // Reduces the best order on the given side (front of the best price
            // level, i.e. the oldest order there under FIFO) by up to max_amount.
            //
            // The reduction is CLAMPED to what the order actually has resting, so
            // the engine can simply ask for the full incoming quantity and read
            // back how much it really got from Fill::quantity. If the order reaches
            // quantity 0 it is removed from the book -- that is the book keeping its
            // own invariant (no zero-quantity orders rest), not a matching decision.
            //
            // Returns std::nullopt if that side is empty or max_amount <= 0; in
            // that case the book is left completely untouched.
            //
            // The returned Fill carries only what the book knows: which maker was
            // reduced, at what price, and by how much. The engine adds taker_id and
            // a timestamp to turn it into a Trade.
            std::optional<Fill> reduce_best_order_quantity(Side side, Quantity amount);

    };
}
