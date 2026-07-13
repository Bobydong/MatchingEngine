#include <chrono>
#include <fstream>
#include <vector>
#include "me/order_book.h"
#include "order_generator.h"


int main() {
    me::OrderBook book;
    me::OrderGenerator gen(42, 10000);

    for (int i = 0; i < 10000; ++i) {
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

    std::vector<long long> latencies;
    latencies.reserve(1'000'000);
    for (int i = 0; i < 1'000'000; ++i) {
        auto op = gen.next();
        if (op.kind == me::GenOp::Kind::Cancel) {
            book.cancel_order(op.cancel_id); 
        }
        else{
            auto t0 = std::chrono::steady_clock::now();
            auto trades = book.add_order(op.order);
            auto t1 = std::chrono::steady_clock::now();
            latencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            // bookkeeping outside timed window
            me::Quantity traded = 0;
            for (auto& t : trades) {
                traded += t.quantity;
                gen.notify_consumed(t.maker_id);
            }
            if (op.kind == me::GenOp::Kind::LimitAdd && traded < op.order.quantity){
                gen.notify_resting(op.order.id);
            }
        }
    }
    // write CSV
    std::ofstream out("benchmarks/results/latency_raw.csv");
    for (auto l : latencies) {
        out << l << "\n";
    }

}
