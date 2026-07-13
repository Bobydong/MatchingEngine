#pragma once
#include "me/order.h"
#include "me/types.h"
#include <random>
#include <vector>

namespace me {
    struct GenOp {
        enum class Kind { LimitAdd, MarketAdd, Cancel };
        Kind kind;
        Order order;      // valid for LimitAdd and MarketAdd
        OrderId cancel_id;  // valid for Cancel
    };

    // Generates a reproducible stream of operations:
    // 60% limit adds, 35% cancels of currently-resting ids, 5% market orders.
    // Prices uniform in [mid-50, mid+50], quantities uniform in [1, 100].
    // When a cancel is due but no orders are resting, falls back to a limit add.
    class OrderGenerator {
        private:
            std::mt19937 rng_;
            Price mid_;
            OrderId next_id_{1};
            Timestamp next_ts_{1};

            std::vector<OrderId> resting_ids_;

            std::uniform_int_distribution<int> roll_{1, 100};
            std::uniform_int_distribution<int> price_dist_{-50, 50};
            std::uniform_int_distribution<int> qty_dist_{1, 100};
            std::uniform_int_distribution<int> side_dist_{0, 1};
        public:
            explicit OrderGenerator(uint32_t seed, Price mid = 10000);
            GenOp next();
            // Inform the generator that an order is now resting in the book.
            void notify_resting(OrderId id);
            // Inform the generator that a resting order was consumed by matching.
            void notify_consumed(OrderId id);

    };
} // namespace me
