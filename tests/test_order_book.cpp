#include <gtest/gtest.h>
#include "me/order_book.h"
#include "me/order.h"

using namespace me;

TEST(OrderBook, EmptyBookReturnsNullopt) {
    OrderBook book;
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.best_ask(), std::nullopt);
}

TEST(OrderBook, AddBidUpdatesBestBid) {
    OrderBook book;
    Order bid{1, Side::Buy, OrderType::Limit, 100, 10, 1};
    book.add_order(bid);
    EXPECT_EQ(book.best_bid(), 100);
    EXPECT_EQ(book.best_ask(), std::nullopt);
}

TEST(OrderBook, HigherBidWins) {
    OrderBook book;
    Order bid_low {1, Side::Buy, OrderType::Limit,  99, 10, 1};
    Order bid_high{2, Side::Buy, OrderType::Limit, 101, 10, 2};
    book.add_order(bid_low);
    book.add_order(bid_high);
    EXPECT_EQ(book.best_bid(), 101);
}

TEST(OrderBook, LowerAskWins) {
    OrderBook book;
    Order ask_high{1, Side::Sell, OrderType::Limit, 102, 10, 1};
    Order ask_low {2, Side::Sell, OrderType::Limit, 100, 10, 2};
    book.add_order(ask_high);
    book.add_order(ask_low);
    EXPECT_EQ(book.best_ask(), 100);
}

TEST(OrderBook, BothSidesCorrectSimultaneously) {
    OrderBook book;
    Order bid{1, Side::Buy,  OrderType::Limit,  99, 10, 1};
    Order ask{2, Side::Sell, OrderType::Limit, 101, 10, 2};
    book.add_order(bid);
    book.add_order(ask);
    EXPECT_EQ(book.best_bid(), 99);
    EXPECT_EQ(book.best_ask(), 101);
}

// T14: single-fill match — incoming buy fully consumes one resting ask
TEST(OrderBook, CrossingLimitFullyFillsRestingOrder) {
    OrderBook book;
    // Resting ask: sell 20 @ 100
    Order ask{1, Side::Sell, OrderType::Limit, 100, 20, 1};
    book.add_order(ask);

    // Incoming buy: buy 20 @ 101 — crosses, should match fully
    Order bid{2, Side::Buy, OrderType::Limit, 101, 20, 2};
    auto trades = book.add_order(bid);

    // One trade emitted
    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].maker_id, 1u);   // resting order is the maker
    EXPECT_EQ(trades[0].taker_id, 2u);   // incoming order is the taker
    EXPECT_EQ(trades[0].price,    100);  // trade at maker's price
    EXPECT_EQ(trades[0].quantity, 20);

    // Book is empty after full match
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.best_bid(), std::nullopt);
}

// T14: remainder rests after partial consumption of liquidity
TEST(OrderBook, RemainderRestsAfterPartialLiquidityConsumed) {
    OrderBook book;
    Order ask{1, Side::Sell, OrderType::Limit, 100, 10, 1};
    book.add_order(ask);

    // Incoming buy wants 30 but only 10 available — 20 should rest
    Order bid{2, Side::Buy, OrderType::Limit, 101, 30, 2};
    auto trades = book.add_order(bid);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 10);

    // Ask side consumed, bid remainder rested at 101
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.best_bid(), 101);
}

// T15: large incoming buy sweeps multiple ask price levels in order
TEST(OrderBook, SweepMultiplePriceLevels) {
    OrderBook book;
    Order ask1{1, Side::Sell, OrderType::Limit, 100, 10, 1};
    Order ask2{2, Side::Sell, OrderType::Limit, 101, 10, 2};
    Order ask3{3, Side::Sell, OrderType::Limit, 102, 10, 3};
    book.add_order(ask1);
    book.add_order(ask2);
    book.add_order(ask3);

    // Buy that sweeps all three levels
    Order bid{4, Side::Buy, OrderType::Limit, 102, 30, 4};
    auto trades = book.add_order(bid);

    // Three trades, one per level, in price order (lowest first)
    ASSERT_EQ(trades.size(), 3u);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[1].price, 101);
    EXPECT_EQ(trades[2].price, 102);
    EXPECT_EQ(trades[0].quantity, 10);
    EXPECT_EQ(trades[1].quantity, 10);
    EXPECT_EQ(trades[2].quantity, 10);

    // All levels consumed, book is empty
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.best_bid(), std::nullopt);
}

// T16: incoming fully consumed, maker has leftover quantity remaining in book
TEST(OrderBook, IncomingFullyConsumedMakerHasRemainder) {
    OrderBook book;
    Order ask{1, Side::Sell, OrderType::Limit, 100, 50, 1};
    book.add_order(ask);

    // Incoming buy wants only 20 — maker should have 30 left
    Order bid{2, Side::Buy, OrderType::Limit, 100, 20, 2};
    auto trades = book.add_order(bid);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 20);

    // Ask level still exists with 30 remaining
    EXPECT_EQ(book.best_ask(), 100);
    EXPECT_EQ(book.best_bid(), std::nullopt);
}

// T17: market buy against empty asks returns no trades and doesn't crash
TEST(OrderBook, MarketBuyEmptyAsksReturnsNoTrades) {
    OrderBook book;
    Order market_buy{1, Side::Buy, OrderType::Market, 0, 20, 1};
    auto trades = book.add_order(market_buy);
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(book.best_bid(), std::nullopt);
    EXPECT_EQ(book.best_ask(), std::nullopt);
}

// T17: market buy fully fills against resting asks
TEST(OrderBook, MarketBuyFullyFills) {
    OrderBook book;
    Order ask{1, Side::Sell, OrderType::Limit, 100, 20, 1};
    book.add_order(ask);

    Order market_buy{2, Side::Buy, OrderType::Market, 0, 20, 2};
    auto trades = book.add_order(market_buy);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].quantity, 20);
    EXPECT_EQ(book.best_ask(), std::nullopt);
}

// T18: limit with quantity exceeding all liquidity — takes all, rests remainder
TEST(OrderBook, LimitExceedsAllLiquidityRestsRemainder) {
    OrderBook book;
    Order ask1{1, Side::Sell, OrderType::Limit, 100, 10, 1};
    Order ask2{2, Side::Sell, OrderType::Limit, 101, 10, 2};
    book.add_order(ask1);
    book.add_order(ask2);

    // Buy wants 50 — only 20 available, 30 should rest at limit price
    Order bid{3, Side::Buy, OrderType::Limit, 102, 50, 3};
    auto trades = book.add_order(bid);

    ASSERT_EQ(trades.size(), 2u);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[1].price, 101);
    EXPECT_EQ(book.best_ask(), std::nullopt);
    EXPECT_EQ(book.best_bid(), 102);  // remainder rested
}

// T18: market buy sweeping multiple levels
TEST(OrderBook, MarketBuySweepsMultipleLevels) {
    OrderBook book;
    Order ask1{1, Side::Sell, OrderType::Limit, 100, 10, 1};
    Order ask2{2, Side::Sell, OrderType::Limit, 101, 10, 2};
    Order ask3{3, Side::Sell, OrderType::Limit, 105, 10, 3};
    book.add_order(ask1);
    book.add_order(ask2);
    book.add_order(ask3);

    Order market_buy{4, Side::Buy, OrderType::Market, 0, 30, 4};
    auto trades = book.add_order(market_buy);

    ASSERT_EQ(trades.size(), 3u);
    EXPECT_EQ(trades[0].price, 100);
    EXPECT_EQ(trades[1].price, 101);
    EXPECT_EQ(trades[2].price, 105);
    EXPECT_EQ(book.best_ask(), std::nullopt);
}

// T18: time priority — two resting orders at same price, older fills first
TEST(OrderBook, TimePriorityOlderOrderFillsFirst) {
    OrderBook book;
    Order ask_first {1, Side::Sell, OrderType::Limit, 100, 10, 1};  // arrived first
    Order ask_second{2, Side::Sell, OrderType::Limit, 100, 10, 2};  // arrived second
    book.add_order(ask_first);
    book.add_order(ask_second);

    // Buy enough to fill only one order
    Order bid{3, Side::Buy, OrderType::Limit, 100, 10, 3};
    auto trades = book.add_order(bid);

    ASSERT_EQ(trades.size(), 1u);
    EXPECT_EQ(trades[0].maker_id, 1u);  // older order matched first
    EXPECT_EQ(book.best_ask(), 100);    // second order still resting
}

// T18: after trade, best_bid/best_ask reflects new state
TEST(OrderBook, BestPricesUpdateAfterTrade) {
    OrderBook book;
    Order ask1{1, Side::Sell, OrderType::Limit, 100, 10, 1};
    Order ask2{2, Side::Sell, OrderType::Limit, 101, 10, 2};
    book.add_order(ask1);
    book.add_order(ask2);
    EXPECT_EQ(book.best_ask(), 100);

    // Consume best ask level
    Order bid{3, Side::Buy, OrderType::Limit, 100, 10, 3};
    book.add_order(bid);

    EXPECT_EQ(book.best_ask(), 101);  // next level is now best
    EXPECT_EQ(book.best_bid(), std::nullopt);
}

// T14: empty price level is erased after full consumption
TEST(OrderBook, EmptyPriceLevelErasedAfterMatch) {
    OrderBook book;
    Order ask1{1, Side::Sell, OrderType::Limit, 100, 10, 1};
    Order ask2{2, Side::Sell, OrderType::Limit, 102, 10, 2};
    book.add_order(ask1);
    book.add_order(ask2);

    // Buy that exactly consumes the best ask level
    Order bid{3, Side::Buy, OrderType::Limit, 100, 10, 3};
    book.add_order(bid);

    // $100 level gone, $102 becomes new best ask
    EXPECT_EQ(book.best_ask(), 102);
}
