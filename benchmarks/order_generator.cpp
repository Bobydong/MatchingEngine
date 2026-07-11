#include "order_generator.h"
#include <algorithm>

namespace me {

    OrderGenerator::OrderGenerator(uint32_t seed, Price mid) : rng_(seed), mid_(mid) {}

    GenOp OrderGenerator::next() {
        int roll = roll_(rng_);
        Side side = side_dist_(rng_) ? Side::Buy : Side::Sell;

        // 35% cancel — fall back to limit add when nothing is resting
        if (roll <= 35 && !resting_ids_.empty()) {
            std::uniform_int_distribution<std::size_t> pick(0, resting_ids_.size() - 1);
            std::size_t idx = pick(rng_);
            OrderId cancel_id = resting_ids_[idx];
            resting_ids_.erase(resting_ids_.begin() + static_cast<std::ptrdiff_t>(idx));
            return GenOp{GenOp::Kind::Cancel, {}, cancel_id};
        }

        // 5% market order (rolls 36-40, or cancel fallback lands here)
        if (roll <= 40) {
            Quantity qty = qty_dist_(rng_);
            Order o{next_id_++, side, OrderType::Market, 0, qty, next_ts_++};
            return GenOp{GenOp::Kind::MarketAdd, o, 0};
        }

        // 60% limit add
        Price price = mid_ + price_dist_(rng_);
        Quantity qty = qty_dist_(rng_);
        Order o{next_id_++, side, OrderType::Limit, price, qty, next_ts_++};
        return GenOp{GenOp::Kind::LimitAdd, o, 0};
    }

    void OrderGenerator::notify_resting(OrderId id) {
        resting_ids_.push_back(id);
    }

    void OrderGenerator::notify_consumed(OrderId id) {
        auto it = std::find(resting_ids_.begin(), resting_ids_.end(), id);
        if (it != resting_ids_.end()) { resting_ids_.erase(it); }
    }

} // namespace me
