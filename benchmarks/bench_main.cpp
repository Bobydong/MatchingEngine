#include <benchmark/benchmark.h>
#include "me/order_book.h"
#include "order_generator.h"

static void BM_AddOrder_SteadyState(benchmark::State& state) {
    me::OrderBook book;
    me::OrderGenerator gen(42, 10000);

    // Populate book to steady state — untimed
    for (int i = 0; i < 5000; ++i) {
        auto op = gen.next();
        if (op.kind == me::GenOp::Kind::Cancel) {
            book.cancel_order(op.cancel_id);
        } else {
            auto trades = book.add_order(op.order);
            me::Quantity traded = 0;
            for (auto& t : trades) {
                traded += t.quantity;
                gen.notify_consumed(t.maker_id);
            }
            if (op.kind == me::GenOp::Kind::LimitAdd &&
                traded < op.order.quantity)
                gen.notify_resting(op.order.id);
        }
    }

    // Pre-generate batch — no allocation inside timed loop
    std::vector<me::Order> orders;
    orders.reserve(100000);
    for (int i = 0; i < 100000; ++i) {
        auto op = gen.next();
        if (op.kind != me::GenOp::Kind::Cancel)
            orders.push_back(op.order);
    }

    std::size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(
            book.add_order(orders[idx % orders.size()]));
        ++idx;
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK(BM_AddOrder_SteadyState);
