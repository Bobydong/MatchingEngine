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
