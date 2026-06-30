# Implementation Brief — C++ Matching Engine (Phases 0 & 1)

**Audience:** This document serves two purposes. It is a specification for Claude Code (or any implementer) to build against, and it is an annotated explanation so the project owner understands the underlying mechanisms — not just the *what* but the *why*. Read the explanation sections carefully; the design decisions encoded in this document are the substance of the interview conversation this project is meant to enable.

**Scope:** Phases 0 (correct core) and 1 (benchmarking). Later phases (optimization, modify/L2 output, backtester) are referenced where they constrain present-day decisions but are not specified here.

---

## Part I — Context & domain primer

### What we're building

A **limit order book and matching engine** — the core software that runs inside a financial exchange. It receives a stream of orders from participants, maintains a sorted view of resting (unmatched) orders, matches incoming orders against resting ones according to strict rules, and emits trade events whenever a match occurs.

The engine is single-instrument (one stock, one currency pair, etc.) and single-threaded. It is invoked synchronously: the caller hands the engine an order, the engine processes it to completion, and returns. There is no networking, no persistence, no GUI. The engine is a pure in-memory data structure with a small public API.

### The objects in the domain

An **order** is a participant's intent to buy or sell. It has:
- An **id** (unique, assigned by the participant)
- A **side**: buy or sell
- A **type**: limit (has a price limit) or market (accepts any available price)
- A **price** (for limit orders only)
- A **quantity** (how much the participant wants to trade)
- A **timestamp** (when it arrived — used for tie-breaking)

The **order book** is the state of all currently-resting (unmatched) orders. It has two sides: the **bid** side (buy orders, sorted highest price first — the highest price is the most aggressive buyer, willing to pay the most) and the **ask** side (sell orders, sorted lowest price first — the lowest price is the most aggressive seller). The **best bid** and **best ask** are the most aggressive prices on each side. The gap between them is the **spread**.

A **trade** is what happens when two orders match. It identifies the two orders involved, the price at which they traded, and the quantity that changed hands.

### The matching rules

When an order arrives, the engine attempts to match it against the opposite side of the book according to **price-time priority**:

1. **Price priority.** Match against the most aggressive resting price first. For an incoming buy, this means the *lowest* ask. For an incoming sell, the *highest* bid.
2. **Time priority.** Among orders at the same price, match the one that arrived earliest first. This is FIFO (first-in, first-out) at each price level.
3. **Continue until either** the incoming order is fully filled, or the next available price would violate its limit (for a limit order), or no opposite orders remain (for a market order).
4. **Whatever remains** of a limit order after matching joins the book as a new resting order at its limit price. Market orders never rest — if they can't be fully matched, the remainder is discarded (or, in real exchanges, rejected; for our purposes, we treat any unfilled market quantity as simply gone).

A **partial fill** occurs when an order matches against multiple resting orders, or when an order's full quantity isn't available at acceptable prices and only some of it trades. Partial fills are the common case, not the exception.

### A worked example

Initial book:

```
Asks: $100.03 — 200 shares (order A3)
      $100.02 — 150 shares (order A2)
      $100.01 — 100 shares (order A1)
      ─────────────────────────────────
Bids: $99.99  — 100 shares (order B1)
      $99.98  — 200 shares (order B2)
```

A new buy limit order arrives: id=X, price=$100.02, quantity=180.

The engine walks the ask side from the lowest price:
- Match 100 shares against A1 at $100.01 → trade(X, A1, $100.01, 100). A1 is fully filled and removed.
- 80 shares of X remain. Match against A2 at $100.02 → trade(X, A2, $100.02, 80). A2 is partially filled; 70 shares remain at $100.02.
- X is fully filled. Done.

Resulting book:

```
Asks: $100.03 — 200 shares (A3)
      $100.02 —  70 shares (A2, partially filled)
      ──────────────────────────────────────────
Bids: $99.99  — 100 shares (B1)
      $99.98  — 200 shares (B2)
```

Two trades were emitted. The book's best ask is now $100.02 with 70 shares. If X had been for 500 shares instead of 180, the engine would have stopped at $100.02 (since $100.03 exceeds its $100.02 limit) after taking 100 + 150 = 250 shares total, and the remaining 250 would have joined the bid side as a new resting order at $100.02 — making *that* the new best bid.

### Why this matters at the data-structure level

Three operations dominate the workload, and each demands a specific property of the data structures:

- **Add an order** that doesn't immediately match: insert into the book at the right price level, at the back of that level's FIFO queue. We want this fast.
- **Match an incoming order**: repeatedly access the best price on the opposite side and the front of its FIFO queue. We want both of these in O(1).
- **Cancel a resting order by id**: locate the order and remove it from its FIFO queue. We want this in O(1), because cancels are extremely common — market makers cancel and re-quote constantly.

Naively, "find an order by id" inside nested price levels is slow. The trick is to maintain a side index: a hash map from order id to a pointer (or iterator) directly into the order's slot in its FIFO queue. With that index plus a FIFO queue type that supports O(1) removal by iterator (`std::list` does; `std::vector` does not), cancels become trivial.

### Why integer prices, not floats

Real exchanges represent prices as integers — typically the count of minimum price increments (ticks). A stock with a $0.01 tick size has price $100.01 stored internally as `10001`. This is **not** an implementation detail you can skip. Floating-point arithmetic introduces rounding errors that make exact equality comparisons unreliable, and order books rely on exact price comparisons for matching. Use integers. We'll define a `Price` type as a 64-bit signed integer representing ticks.

---

## Part II — High-level goals

This section captures *what success looks like* before we descend into specification.

### Phase 0 goal

A working, correct, well-tested matching engine that anyone can clone, build with one command, and run. Correctness is verified two ways: hand-written unit tests covering specific scenarios, and randomized property tests that compare the real engine against a deliberately simple "reference" implementation.

Phase 0 is **not** about performance. Use standard library containers throughout. Allocate freely. The goal is a foundation correct enough that Phase 2's optimizations can be aggressive without fear of breaking behavior.

### Phase 1 goal

A repeatable benchmark harness that measures the engine's throughput (orders per second) and latency distribution (p50, p99, p99.9). The harness uses synthetic order flow with a deterministic seed so results are reproducible. Output is a CSV plus a markdown summary table.

Phase 1 produces the **baseline** that Phase 2 will improve. The baseline must be honest: warmed-up, statistically stable, and measured under conditions you can describe precisely in an interview.

### What "done" looks like end-to-end

A public GitHub repository where:
- `git clone && cd repo && cmake -B build && cmake --build build && ctest --test-dir build` produces a green test run.
- A `bench` target runs the benchmark suite and prints a results summary.
- The README explains the design, shows the baseline numbers, and walks through one or two non-obvious decisions (e.g., why std::map, why the order-id index).

---

## Part III — Architecture overview

### Component layout

The system has four layers, from low-level to high:

1. **Domain types** (`types.h`, `order.h`, `trade.h`). Plain data structures — `Price`, `Quantity`, `OrderId`, `Side`, `OrderType`, `Order`, `Trade`. No logic, just shapes.

2. **OrderBook** (`order_book.h/cpp`). The core data structure. Holds the bids and asks, exposes operations to add, cancel, and (internally) match. Emits trades as a return value when matching occurs.

3. **MatchingEngine** (`matching_engine.h/cpp`). A thin layer over OrderBook that owns one book per instrument (just one in Phase 0), accepts a stream of order events, dispatches them to the book, and collects the resulting trades. This is the public entry point.

4. **I/O & harnesses**. CSV reader for replaying recorded order streams (useful for tests). Random order generator for benchmarks. These live outside the engine proper.

The separation between `OrderBook` and `MatchingEngine` may feel artificial in Phase 0, but it pays off when Phase 3 adds multiple instruments — at that point `MatchingEngine` becomes the dispatcher to a collection of `OrderBook` instances keyed by instrument id.

### Public API (conceptual, finalized below)

```cpp
struct Order { /* id, side, type, price, qty, timestamp */ };
struct Trade { /* maker_id, taker_id, price, qty, timestamp */ };

class OrderBook {
public:
    std::vector<Trade> add_order(const Order& o);
    bool cancel_order(OrderId id);
    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    // Phase 3: modify_order, snapshot
};
```

`add_order` is the workhorse. It accepts an order, performs all matching, returns the list of trades that resulted, and (if any quantity remains and it's a limit order) leaves the remainder resting in the book.

---

## Part IV — Detailed design (Phase 0)

### Domain types

```cpp
// types.h
#include <cstdint>

using Price    = std::int64_t;   // in ticks
using Quantity = std::int64_t;
using OrderId  = std::uint64_t;
using Timestamp = std::uint64_t; // monotonic counter or chrono::nanoseconds

enum class Side : std::uint8_t  { Buy, Sell };
enum class OrderType : std::uint8_t { Limit, Market };
```

**Why these specific choices:** `int64_t` for prices gives effectively unlimited range in ticks; signed is helpful so that "no price" can be a sentinel if needed, though we prefer `std::optional<Price>` for that. `uint64_t` for ids ensures we never run out in benchmarks that submit billions of orders. Fixed-width types throughout — never `int` or `long`, whose sizes vary by platform and produce subtle bugs.

```cpp
// order.h
struct Order {
    OrderId   id;
    Side      side;
    OrderType type;
    Price     price;       // ignored for Market orders
    Quantity  quantity;    // total quantity at submission
    Timestamp timestamp;
};
```

```cpp
// trade.h
struct Trade {
    OrderId   maker_id;    // the resting order
    OrderId   taker_id;    // the incoming order
    Price     price;
    Quantity  quantity;
    Timestamp timestamp;
};
```

**Maker vs taker terminology:** the "maker" is the order that was already resting in the book (made the liquidity). The "taker" is the incoming order that consumed it. Trade prices are always at the maker's price — this is a fundamental matching rule and worth internalizing. When you submit a buy at $100.02 and it matches a resting sell at $100.01, the trade is at $100.01, not $100.02. The taker gets price improvement; the maker gets exactly what they posted.

### OrderBook data structures

Internally, `OrderBook` holds:

```cpp
class OrderBook {
private:
    // Each price level is a FIFO of orders at that price.
    // std::list gives O(1) insertion at back and O(1) removal by iterator.
    using PriceLevel = std::list<Order>;

    // Bids sorted high-to-low: greater<Price> as comparator.
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    // Asks sorted low-to-high: default less<Price>.
    std::map<Price, PriceLevel, std::less<Price>> asks_;

    // For O(1) cancel: locate an order by id.
    struct OrderLocation {
        Side side;
        Price price;
        std::list<Order>::iterator iter;
    };
    std::unordered_map<OrderId, OrderLocation> index_;
};
```

**Why std::map for price levels:** ordered, supports O(log n) insertion and O(1) access to the smallest or largest key via `begin()`/`rbegin()`. For Phase 0, this is correct and clear. It's also slow in absolute terms — a node-based balanced tree with poor cache behavior — and Phase 2 will likely replace it with a flat sorted structure (sorted vector of price levels, or an array indexed by ticks-from-mid). But that's premature now. Document the choice in a comment so future-you remembers it's intentional.

**Why std::list for the FIFO queue:** we need O(1) insertion at the back (for adding a new resting order) and O(1) removal at arbitrary position (for cancel-by-id, given an iterator). `std::list` satisfies both. `std::deque` has O(1) push_back but not O(1) middle removal. `std::vector` is even worse for middle removal. The cost is a heap allocation per node — fine in Phase 0, optimization target in Phase 2 (intrusive lists with pre-allocated node pools).

**Why the order-id index stores an iterator:** because given the iterator, `std::list::erase(iter)` is O(1). This is the entire reason we use `std::list` — it's the only standard container where iterators remain valid through other insertions and removals, and where erase-by-iterator is constant time. If you ever switch to a container that invalidates iterators on modification (vector, deque), this whole pattern breaks.

### The matching algorithm

`add_order` is the heart of the engine. Pseudocode:

```
function add_order(incoming):
    trades = []
    if incoming.side == Buy:
        opposite_side = asks_
        crosses(level_price, limit) = (incoming is market) or (level_price <= limit)
    else:
        opposite_side = bids_
        crosses(level_price, limit) = (incoming is market) or (level_price >= limit)

    while incoming.quantity > 0 and opposite_side is not empty:
        best_level_price = first key in opposite_side
        if not crosses(best_level_price, incoming.price):
            break

        queue = opposite_side[best_level_price]
        while incoming.quantity > 0 and queue is not empty:
            resting = queue.front()
            traded_qty = min(incoming.quantity, resting.quantity)
            trades.append(Trade{
                maker_id = resting.id,
                taker_id = incoming.id,
                price    = best_level_price,
                quantity = traded_qty,
                timestamp = incoming.timestamp
            })
            incoming.quantity -= traded_qty
            resting.quantity -= traded_qty
            if resting.quantity == 0:
                index_.erase(resting.id)
                queue.pop_front()

        if queue is empty:
            opposite_side.erase(best_level_price)

    if incoming.quantity > 0 and incoming.type == Limit:
        // Rest the remainder.
        side_map = (incoming.side == Buy) ? bids_ : asks_
        queue = side_map[incoming.price]   // creates if missing
        queue.push_back(incoming)
        iter = std::prev(queue.end())
        index_[incoming.id] = {incoming.side, incoming.price, iter}

    return trades
```

**Key invariants the implementation must preserve:**

1. After every `add_order` call, the index map's contents exactly correspond to the orders resting in the book — no stale entries, no missing ones.
2. The book never contains empty price levels. When a level's queue becomes empty (because all its orders matched or were canceled), the level itself must be erased from the map. Otherwise `best_bid()` / `best_ask()` will return phantom prices.
3. The book never contains crossed prices — best bid is always strictly less than best ask. This follows automatically from correct matching: any incoming order that would cross is matched first, so it can never *rest* at a crossing price.
4. Quantities are always positive in resting orders. An order whose quantity reaches zero is removed entirely, not left behind with zero.

These invariants are what your tests will check.

### Cancellation

```
function cancel_order(id):
    if id not in index_:
        return false
    location = index_[id]
    side_map = (location.side == Buy) ? bids_ : asks_
    queue = side_map[location.price]
    queue.erase(location.iter)        // O(1)
    if queue is empty:
        side_map.erase(location.price)
    index_.erase(id)
    return true
```

**Why this is O(1):** the index gives us the iterator directly. The list erase is constant time. The only sub-O(1) thing is the map lookup (`side_map[location.price]`), which is O(log n) in the number of price levels — but that's typically small (hundreds at most) and disappears in the noise of a real benchmark.

A subtle Phase 2 optimization: avoid the price-level map lookup entirely by storing the *price level pointer* (or list reference) in the index, not just the price. We don't need to do this in Phase 0 — clarity wins.

### Best-price queries

```cpp
std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;   // greatest, because std::greater<Price>
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;   // smallest, default less<Price>
}
```

Both are O(log 1) = O(1) in practice — `std::map::begin()` is constant time. Returning `std::optional` is cleaner than a sentinel value and forces callers to handle the empty-book case.

### Edge cases the implementation must handle

- **Self-trade** (incoming order from the same participant as a resting order). In Phase 0, allow it — we have no concept of participant ids yet. Note this as a known limitation in the README.
- **Zero-quantity order submitted.** Reject by ignoring; don't crash. A debug-mode assertion is fine.
- **Duplicate order id.** The behavior is unspecified by real exchanges (they reject). For Phase 0, ignore the duplicate and emit no trades — log it in debug builds.
- **Cancel of an unknown id.** Return `false`. Do not throw.
- **Market order against an empty opposite side.** Returns no trades, no remainder rests, function exits cleanly.
- **Crossing market order with limited liquidity.** Match as much as possible; the remainder is silently dropped (don't rest market orders).

### Testing strategy

Phase 0 testing has three layers:

**Unit tests** (hand-written, deterministic). Cover each behavior in isolation. Suggested coverage:

- Adding a non-crossing limit order rests it in the book; best bid/ask reflect it.
- Adding two limit orders at the same price preserves time priority (the first one added is matched first).
- A crossing limit order matches against one resting order fully and exits.
- A crossing limit order matches against multiple price levels (sweeps the book).
- A crossing limit order with leftover quantity rests the remainder at its limit price.
- A market order with no opposite liquidity returns no trades.
- A market order matches across multiple levels until filled.
- Cancel of a resting order removes it; best bid/ask update.
- Cancel of an unknown id returns false.
- After cancel, the canceled order does not participate in subsequent matches.
- Partial fills: the maker's remaining quantity is correct after a partial match.
- Empty price levels are pruned (use `best_bid()`/`best_ask()` to detect phantom levels).

**Reference matcher** (deliberately simple, deliberately slow). A separate implementation living in `tests/reference_matcher.h/cpp`. Same public API as `OrderBook`, but implemented in the most obviously-correct way possible — e.g., bids and asks as sorted `std::vector<Order>`, full linear scans on every operation. This is your oracle. It does not need to be fast; it needs to be unambiguously correct.

**Randomized property tests.** Generate a long stream of random orders (add/cancel mix, random sides, random prices in a small range so things actually match). Apply the stream to both the real engine and the reference matcher. After every operation, assert:

- The list of trades emitted is identical.
- The set of resting order ids is identical.
- For every resting order id, the quantity is identical.
- The best bid and best ask agree.

Run with thousands of orders per test, multiple seeds. This is what catches the bugs unit tests miss. The whole reason to keep `OrderBook` decoupled from I/O is so this test setup is trivial.

### File layout

```
matching-engine/
├── CMakeLists.txt
├── README.md
├── include/
│   └── me/                           # "me" = matching engine
│       ├── types.h
│       ├── order.h
│       ├── trade.h
│       ├── order_book.h
│       └── matching_engine.h
├── src/
│   ├── order_book.cpp
│   └── matching_engine.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── reference_matcher.h
│   ├── reference_matcher.cpp
│   ├── test_order_book.cpp
│   ├── test_matching_engine.cpp
│   └── test_randomized.cpp
└── bench/                            # Phase 1
    ├── CMakeLists.txt
    ├── order_generator.h
    ├── order_generator.cpp
    └── bench_main.cpp
```

---

## Part V — Build system

### Top-level CMakeLists.txt

The build should:

- Require CMake 3.20+ (modern enough for `FetchContent` to be ergonomic).
- Set C++20 as the standard (`-std=c++20`), with `CMAKE_CXX_EXTENSIONS OFF` for portability.
- Default to Release build type (`-O3 -DNDEBUG`) if none specified; Debug builds add `-g -O0 -fsanitize=address,undefined`.
- Enable warnings as a baseline: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wnon-virtual-dtor`. Do **not** use `-Werror` in the project itself — recruiters cloning the repo on a different compiler version may hit warnings unrelated to your code. Treat warnings as errors only in CI.
- Pull in GoogleTest and Google Benchmark via `FetchContent` (pinned to specific versions, not master).
- Expose targets: `me_core` (static library), `me_tests` (test runner), `me_bench` (benchmark runner). Register tests with CTest via `gtest_discover_tests`.

A minimal viable CMakeLists.txt is roughly:

```cmake
cmake_minimum_required(VERSION 3.20)
project(matching_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wconversion)

# Sanitizers in Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Core library
add_library(me_core
    src/order_book.cpp
    src/matching_engine.cpp
)
target_include_directories(me_core PUBLIC include)

# Tests
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)
add_subdirectory(bench)
```

Subdirectory CMakeLists.txt files in `tests/` and `bench/` define their respective executables and link against `me_core` + the relevant framework.

### Quality-of-life

- `.clang-format` file at repo root (use Google or LLVM style — pick one, commit it, don't bikeshed). Run formatting before every commit.
- `.gitignore` covering `build/`, `*.o`, editor files, etc.
- Optional: a GitHub Actions workflow that builds and tests on push. Not required for Phase 0 but useful for showing professionalism.

---

## Part VI — Phase 1: Benchmarking

### What we measure and why

There are two related but distinct things to measure:

**Throughput** — orders processed per unit time. This is what you quote as "X million orders per second." Measured by feeding a large batch of pre-generated orders through the engine and dividing batch size by elapsed time. Important: include only the engine's work in the timed region. Pre-generate the orders, then start the clock, run the loop, stop the clock.

**Latency distribution** — how long a single `add_order` call takes, measured across many calls and reported as percentiles. The median (p50) tells you the typical case. The tail (p99, p99.9) tells you the worst common case. **Averages are almost useless for low-latency systems** because they hide the tail: a single 1ms outlier in a sea of 1µs operations barely moves the mean but is exactly the kind of behavior that gets you fired at a trading firm. Always quote percentiles.

### The timing primitive

```cpp
auto t0 = std::chrono::steady_clock::now();
book.add_order(order);
auto t1 = std::chrono::steady_clock::now();
auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
```

`steady_clock` is monotonic (never goes backward), has ~nanosecond resolution on Linux, and has overhead of ~20-30ns per call. The overhead matters because at scale you're measuring operations that themselves take only a few hundred nanoseconds. We accept this overhead in Phase 1; if it becomes a problem in Phase 2, the move is to use `__rdtsc` directly or to batch multiple operations under a single timer.

### Pre-allocation discipline

Before the timed region, allocate everything: the vector of orders to feed in, the vector to collect trade outputs, any test-side state. The benchmark loop should do *zero* heap allocation other than what the engine itself does internally. Otherwise you're benchmarking malloc, not the engine.

### Warmup

The first thousand or so operations against a fresh engine hit cold caches, an empty index map (which then resizes), and uninitialized branch predictors. These early measurements are not representative. Standard practice: run a warmup phase (say, 10,000 operations) before starting measurement, and discard those samples. Google Benchmark handles this automatically; if you roll your own, do it explicitly.

### Steady-state book

The book's behavior depends on its size. An empty book matches nothing; every order rests. A deeply populated book matches frequently and exercises the matching loop. You want measurements taken against a **representative** book state — something with hundreds to thousands of resting orders distributed across dozens of price levels.

Approach: before timing, run a "fill" phase that submits a few thousand orders to populate the book to a target depth. Then begin measurement. The synthetic generator (below) is tuned so the book stays roughly in steady state during measurement — adds and cancels and matches balance out.

### The synthetic order generator

A configurable component that emits a stream of orders matching a chosen distribution. Parameters:

- **Add/cancel/market ratio** — e.g., 70% limit adds, 25% cancels, 5% market orders. Real markets see vastly more adds and cancels than fills; market makers churn constantly.
- **Price distribution** — typically a Gaussian centered on a "mid" price, with a tight standard deviation. Spreading orders across too wide a range means nothing ever matches; too narrow means everything matches immediately.
- **Quantity distribution** — log-normal is realistic; uniform-over-small-range is fine for a starting point.
- **Cancel selection** — randomly chosen from currently-resting orders. Maintain a side structure tracking what's resting so you can pick valid ids.
- **Seed** — explicit, so the entire stream is reproducible.

Recommended starting parameters: 60% limit adds, 35% cancels, 5% market orders; prices uniform in `[mid-50, mid+50]` ticks; quantities uniform in `[1, 100]`; mid = 10000. Run with seed=42 for the canonical baseline; run with several other seeds to confirm stability.

### Benchmark harness structure

Using Google Benchmark, the high-level shape is:

```cpp
static void BM_AddOrder_SteadyState(benchmark::State& state) {
    OrderBook book;
    OrderGenerator gen(/*seed=*/42, /*mid=*/10000);
    
    // Populate to steady state.
    for (int i = 0; i < 5000; ++i) {
        book.add_order(gen.next());
    }
    
    // Pre-generate the batch we'll iterate over.
    std::vector<Order> orders;
    orders.reserve(100000);
    for (int i = 0; i < 100000; ++i) {
        orders.push_back(gen.next());
    }
    
    size_t idx = 0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(book.add_order(orders[idx % orders.size()]));
        ++idx;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddOrder_SteadyState);
```

`benchmark::DoNotOptimize` is a barrier preventing the compiler from optimizing away the call (since the result might be ignored). Without it, an aggressive optimizer can delete the work you're trying to measure.

Google Benchmark reports mean, median, stddev. To get p99/p99.9 you need either a manual histogram or a separate latency-distribution harness. Suggested approach: in addition to the BM_* benchmarks, write a standalone `latency_dist` executable that runs N operations, records every individual latency, and computes percentiles offline. Output to CSV.

### Reporting format

In the README, present a table:

```markdown
## Benchmark Results — Baseline (Phase 1)

Hardware: Intel i7-12700K @ 3.6 GHz, 32 GB DDR4, Ubuntu 24.04, GCC 13.2, -O3
Methodology: 1M operations, steady-state book (~5000 resting), seed=42

| Operation       | p50    | p99    | p99.9   | Throughput |
|-----------------|--------|--------|---------|------------|
| add_order       | XXX ns | XXX ns | XXX ns  | XX M/sec   |
| cancel_order    | XXX ns | XXX ns | XXX ns  | XX M/sec   |
```

Be specific about the conditions. An interviewer will absolutely ask "what hardware" and "what compiler flags" — having the answer written down means you can speak with confidence.

### Things that go wrong in benchmarking

- **Forgetting to disable CPU frequency scaling.** On Linux: `sudo cpupower frequency-set -g performance`. Without this, your CPU clocks down between iterations and your numbers are noisy.
- **Background load.** Run benchmarks on an otherwise-idle machine. Close the browser.
- **Thermal throttling.** Long benchmark runs heat the CPU; modern CPUs throttle. If consecutive runs produce drifting numbers, this is likely the cause.
- **Not pinning to a single core.** Cross-core migration adds noise. `taskset -c 2 ./me_bench` pins to CPU 2.
- **Measuring with a debug build.** Sanitizers and `-O0` give numbers 10-50x slower than release. Always benchmark `-O3 -DNDEBUG`.

These environmental factors are not Phase 1's main concern — first get the harness working — but mention them in the README. Demonstrating awareness of measurement methodology is itself a signal.

---

## Part VII — Build order & milestones

This is the suggested execution sequence. Each milestone ends in a state where the project is shippable — tests pass, builds clean, README reflects current scope.

**M0.1 — Project skeleton.** CMakeLists.txt, directory structure, empty files, a hello-world test that confirms GoogleTest is wired up. Commit and push.

**M0.2 — Domain types & Order.** Define types.h, order.h, trade.h. No logic, just the structs. Write a trivial test that constructs an Order and a Trade and asserts field values. This gets type definitions reviewed before any algorithm depends on them.

**M0.3 — OrderBook: add non-crossing limit orders.** Implement `add_order` for the case where the order does not cross. It just inserts into the right side at the right level. Implement `best_bid` and `best_ask`. Test that adding orders updates these correctly and preserves price ordering.

**M0.4 — OrderBook: matching crossing orders.** Extend `add_order` to handle the matching loop. Test single-level matches, multi-level sweeps, partial fills, market orders. This is the most algorithmically dense milestone; expect debugging.

**M0.5 — OrderBook: cancel.** Implement the order-id index and `cancel_order`. Test cancels in isolation and in combination with matches. Confirm canceled orders are not visible to subsequent matches.

**M0.6 — Reference matcher & randomized tests.** Build the simple slow reference. Write the randomized cross-check test. Run with many seeds. **This milestone is where you'll find bugs in M0.4 and M0.5 — expect to circle back.** When this is green at 10,000+ operations across 20+ seeds, the engine is correct.

**M0.7 — MatchingEngine layer & cleanup.** Add the thin `MatchingEngine` wrapper. Polish the README. Tag a `v0.1` release. Phase 0 complete.

**M1.1 — Order generator.** Build the synthetic stream generator. Test it in isolation (correct distributions, reproducible with the same seed).

**M1.2 — Google Benchmark integration.** Wire up Google Benchmark via FetchContent. Get one `BM_AddOrder` benchmark running end-to-end.

**M1.3 — Latency distribution harness.** Build the separate executable that records per-operation latencies and dumps a CSV. Add a small Python script (or markdown table by hand) to compute and present percentiles.

**M1.4 — Baseline measurement.** Run the full suite under controlled conditions (release build, CPU pinned, scaling disabled). Capture results into a `bench/results/baseline.md` file committed to the repo. Update the main README's results table.

**M1.5 — Documentation pass.** Write up the design and the baseline numbers in the README. Include the "what's next" section pointing at the Phase 2 optimization plan. Tag `v0.2`. Phase 1 complete.

After M1.5, you have a project that is on its own a strong portfolio piece. Phase 2 (optimization) is where the real differentiation happens, and the work above is what enables it: an engine you trust to be correct, and numbers you trust to be honest.

---

## Part VIII — Conventions & expectations for the implementer

For Claude Code (or whoever is writing the actual code):

- **Tests-first where feasible.** Especially for the matching algorithm in M0.4 — write the test cases before the implementation. The test cases double as a specification.
- **Small commits.** Each milestone above is multiple commits. Commit messages in present tense, imperative ("add cancel_order with index lookup"), not past tense.
- **Comments explain *why*, not *what*.** `// loop over orders` is noise. `// must remove empty price levels to keep best_bid() honest` is useful.
- **No premature abstraction.** Single instrument means single instrument. Don't templatize on price type, don't add a "strategy pattern" for matching algorithms, don't introduce a logger framework. Plain code that does one thing.
- **No dependencies beyond stdlib + GoogleTest + Google Benchmark.** No Boost, no fmt, no spdlog. If you find yourself reaching for one, stop and ask why.
- **`const` correctness throughout.** `best_bid() const`, references to `const Order&` in interfaces. This is C++; it matters.
- **No exceptions on the hot path.** `add_order` and `cancel_order` should not throw. Return values communicate failure (`bool` from cancel, empty vector from add).
- **No raw `new`/`delete`.** Standard containers handle their own memory in Phase 0. Phase 2 will introduce object pools, but those are RAII-managed.

---

## Part IX — What the README should look like at v0.2

When Phase 1 is done, the repository's README should communicate, in order:

1. **One-paragraph project description.** What this is, who it's for, what's interesting about it.
2. **Quick start.** Three commands: clone, build, test. Plus one to run benchmarks.
3. **Design overview.** Half a page on the data structures and the matching algorithm. Reference price-time priority. Mention the order-id index.
4. **Benchmark results table.** Hardware, methodology, p50/p99/p99.9, throughput.
5. **Project status / roadmap.** Phase 0 done, Phase 1 done, Phase 2 (optimization) next. Honest about what's done.
6. **Design decisions worth calling out.** A short list of "I chose X because Y, with the tradeoff Z." This is what interviewers will read. Examples: why std::map; why std::list for the FIFO queue; why integer prices; why a separate reference matcher.

A future reader (recruiter, interviewer, you in three months) should be able to read this README in five minutes and understand both what the project does and why specific design choices were made. That readability is itself part of the deliverable.
