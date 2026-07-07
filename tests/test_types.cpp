#include <gtest/gtest.h>
#include "me/order.h"
#include "me/trade.h"

using namespace me;

TEST(Types, OrderAggregateInit) {
    Order o{1, Side::Buy, OrderType::Limit, 10001, 50, 100};
    EXPECT_EQ(o.id,        1u);
    EXPECT_EQ(o.side,      Side::Buy);
    EXPECT_EQ(o.type,      OrderType::Limit);
    EXPECT_EQ(o.price,     10001);
    EXPECT_EQ(o.quantity,  50);
    EXPECT_EQ(o.timestamp, 100u);
}

TEST(Types, TradeAggregateInit) {
    Trade t{42, 99, 10001, 25, 200};
    EXPECT_EQ(t.maker_id,  42u);
    EXPECT_EQ(t.taker_id,  99u);
    EXPECT_EQ(t.price,     10001);
    EXPECT_EQ(t.quantity,  25);
    EXPECT_EQ(t.timestamp, 200u);
}

TEST(Types, SideAndOrderTypeValues) {
    Order buy_limit{1, Side::Buy,  OrderType::Limit,  10000, 10, 1};
    Order sell_mkt {2, Side::Sell, OrderType::Market, 0,     10, 2};
    EXPECT_NE(buy_limit.side, sell_mkt.side);
    EXPECT_NE(buy_limit.type, sell_mkt.type);
}
