#include <gtest/gtest.h>
#include "order_generator.h"

using namespace me;

TEST(OrderGenerator, SameSeedProducesIdenticalStream) {
    OrderGenerator g1(42);
    OrderGenerator g2(42);

    for (int i = 0; i < 1000; ++i) {
        GenOp op1 = g1.next();
        GenOp op2 = g2.next();

        ASSERT_EQ(op1.kind, op2.kind) << "kind mismatch at i=" << i;
        if (op1.kind == GenOp::Kind::Cancel) {
            EXPECT_EQ(op1.cancel_id, op2.cancel_id);
        } else {
            EXPECT_EQ(op1.order.side,     op2.order.side);
            EXPECT_EQ(op1.order.type,     op2.order.type);
            EXPECT_EQ(op1.order.price,    op2.order.price);
            EXPECT_EQ(op1.order.quantity, op2.order.quantity);
        }
    }
}

TEST(OrderGenerator, DifferentSeedsProduceDifferentStreams) {
    OrderGenerator g1(42);
    OrderGenerator g2(99);

    int differences = 0;
    for (int i = 0; i < 100; ++i) {
        GenOp op1 = g1.next();
        GenOp op2 = g2.next();
        if (op1.kind != op2.kind ||
            (op1.kind != GenOp::Kind::Cancel &&
             op1.order.price != op2.order.price))
            ++differences;
    }
    EXPECT_GT(differences, 0) << "two different seeds produced identical streams";
}

TEST(OrderGenerator, DistributionWithinExpectedRange) {
    OrderGenerator gen(42);

    int limit_adds  = 0;
    int market_adds = 0;
    int cancels     = 0;
    const int N = 10000;

    for (int i = 0; i < N; ++i) {
        GenOp op = gen.next();
        switch (op.kind) {
            case GenOp::Kind::LimitAdd:  ++limit_adds;  gen.notify_resting(op.order.id); break;
            case GenOp::Kind::MarketAdd: ++market_adds; break;
            case GenOp::Kind::Cancel:    ++cancels;     break;
        }
    }

    double add_pct = 100.0 * limit_adds / N;
    EXPECT_GE(add_pct, 55.0) << "limit add % too low: " << add_pct;
    EXPECT_LE(add_pct, 75.0) << "limit add % too high: " << add_pct;

    EXPECT_GT(cancels,     0) << "no cancels generated";
    EXPECT_GT(market_adds, 0) << "no market orders generated";
}
