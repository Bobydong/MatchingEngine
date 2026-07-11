#include <gtest/gtest.h>
#include "me/order_book.h"
#include "reference_matcher.h"
#include <algorithm>
#include <random>
#include <unordered_map>
#include <vector>

using namespace me;

static void run_randomized_test(uint64_t seed) {
    OrderBook book;
    ReferenceMatcher ref;

    std::mt19937 rng(static_cast<uint32_t>(seed));
    std::uniform_int_distribution<int> roll_dist(1, 100);
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_int_distribution<int> price_dist(-50, 50);
    std::uniform_int_distribution<int> qty_dist(1, 100);

    const Price mid = 10000;

    // Track resting orders and remaining quantities for valid cancel selection
    std::unordered_map<OrderId, Quantity> resting_qty;
    std::vector<OrderId> resting_ids;

    OrderId next_id = 1;

    for (int i = 0; i < 5000; ++i) {
        int roll = roll_dist(rng);
        std::vector<Trade> t1, t2;

        if (roll <= 30 && !resting_ids.empty()) {
            // Cancel a random resting order
            std::uniform_int_distribution<int> pick(0, (int)resting_ids.size() - 1);
            int idx = pick(rng);
            OrderId cancel_id = resting_ids[idx];

            bool r1 = book.cancel_order(cancel_id);
            bool r2 = ref.cancel_order(cancel_id);
            ASSERT_EQ(r1, r2) << "cancel result mismatch id=" << cancel_id << " seed=" << seed << " iter=" << i;

            resting_qty.erase(cancel_id);
            resting_ids.erase(resting_ids.begin() + idx);

        } else if (roll <= 40) {
            // Market order
            Side side = side_dist(rng) ? Side::Buy : Side::Sell;
            Quantity qty = qty_dist(rng);
            OrderId id = next_id++;
            Order order{id, side, OrderType::Market, 0, qty, (Timestamp)i};

            t1 = book.add_order(order);
            t2 = ref.add_order(order);

        } else {
            // Limit add
            Side side = side_dist(rng) ? Side::Buy : Side::Sell;
            Price price = mid + price_dist(rng);
            Quantity qty = qty_dist(rng);
            OrderId id = next_id++;
            Order order{id, side, OrderType::Limit, price, qty, (Timestamp)i};

            t1 = book.add_order(order);
            t2 = ref.add_order(order);

            // Track remainder — qty minus what was traded by incoming
            Quantity traded = 0;
            for (const Trade& t : t1) traded += t.quantity;
            Quantity remaining = qty - traded;
            if (remaining > 0) {
                resting_qty[id] = remaining;
                resting_ids.push_back(id);
            }
        }

        // Assert trades match
        ASSERT_EQ(t1.size(), t2.size())
            << "trade count mismatch seed=" << seed << " iter=" << i;
        for (size_t j = 0; j < t1.size(); ++j) {
            EXPECT_EQ(t1[j].maker_id, t2[j].maker_id) << "maker_id mismatch seed=" << seed << " iter=" << i;
            EXPECT_EQ(t1[j].price,    t2[j].price)    << "price mismatch";
            EXPECT_EQ(t1[j].quantity, t2[j].quantity) << "quantity mismatch";
        }

        // Update resting set: remove makers that were fully consumed
        for (const Trade& t : t1) {
            auto it = resting_qty.find(t.maker_id);
            if (it != resting_qty.end()) {
                it->second -= t.quantity;
                if (it->second <= 0) {
                    resting_qty.erase(it);
                    resting_ids.erase(
                        std::find(resting_ids.begin(), resting_ids.end(), t.maker_id));
                }
            }
        }

        // Assert best bid/ask agree after every operation
        ASSERT_EQ(book.best_bid(), ref.best_bid())
            << "best_bid mismatch seed=" << seed << " iter=" << i;
        ASSERT_EQ(book.best_ask(), ref.best_ask())
            << "best_ask mismatch seed=" << seed << " iter=" << i;
    }
}

TEST(Randomized, Seed42)    { run_randomized_test(42); }
TEST(Randomized, Seed1)     { run_randomized_test(1); }
TEST(Randomized, Seed7)     { run_randomized_test(7); }
TEST(Randomized, Seed100)   { run_randomized_test(100); }
TEST(Randomized, Seed12345) { run_randomized_test(12345); }
