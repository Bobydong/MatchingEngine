#include <gtest/gtest.h>
#include "me/matching_engine.h"

using namespace me;

TEST(MatchingEngine, MatchProducesTrade) {
    MatchingEngine engine;

    Order ask{1, Side::Sell, OrderType::Limit, 100, 10, 1};
    engine.add_order(ask);

    Order bid{2, Side::Buy, OrderType::Limit, 100, 10, 2};
    auto trades = engine.add_order(bid);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].maker_id, 1u);
    EXPECT_EQ(trades[0].taker_id, 2u);
    EXPECT_EQ(trades[0].price,    100);
    EXPECT_EQ(trades[0].quantity, 10);
    EXPECT_EQ(engine.best_ask(), std::nullopt);
    EXPECT_EQ(engine.best_bid(), std::nullopt);
}

TEST(MatchingEngine, CancelRemovesOrder) {
    MatchingEngine engine;

    Order bid{1, Side::Buy, OrderType::Limit, 99, 10, 1};
    engine.add_order(bid);
    EXPECT_EQ(engine.best_bid(), 99);

    EXPECT_TRUE(engine.cancel_order(1u));
    EXPECT_EQ(engine.best_bid(), std::nullopt);

    // Canceled order produces no trade when a crossing order arrives
    Order ask{2, Side::Sell, OrderType::Limit, 99, 10, 2};
    auto trades = engine.add_order(ask);
    EXPECT_TRUE(trades.empty());
}
