# Action Plan — C++ Matching Engine (Phases 0 & 1)

**How to use this document.** Each task below is a self-contained unit of work — typically 1–4 hours, sometimes less. Do them in order; later tasks depend on earlier ones. Each task tells you what to build, how to approach it, and what to watch out for. When you finish a task, commit your work with a message describing what you did, then move to the next.

This is a working checklist, not a brief for AI. You're writing this yourself. Use AI for explanations, code review, and getting unstuck — not for generating the engine code itself. The earlier `IMPLEMENTATION_BRIEF.md` is your reference for *why* things are designed this way; this document is your reference for *what to do next*.

---

## Milestone 0.1 — Project skeleton

### Task 1: Create the directory structure

**Goal.** A repository with the layout from the brief, empty placeholder files in place, ready to receive code.

**Approach.** Create a new directory `matching-engine/`. Inside it create `include/me/`, `src/`, `tests/`, `bench/`. Create empty (or single-comment-line) versions of `include/me/types.h`, `include/me/order.h`, `include/me/trade.h`, `include/me/order_book.h`, `include/me/matching_engine.h`, `src/order_book.cpp`, `src/matching_engine.cpp`. Create a top-level `README.md` with just the project name for now.

**Watch out for.** Don't put `.cpp` files in `include/`. Headers in `include/`, implementations in `src/`. Get this right now so you don't have to fix it later.

### Task 2: Initialize git and .gitignore

**Goal.** Repository initialized, .gitignore in place, first commit made.

**Approach.** Run `git init`. Create a `.gitignore` covering `build/`, `*.o`, `*.a`, `.vscode/`, `.idea/`, `cmake-build-*/`, `compile_commands.json`, and OS-specific files (`.DS_Store`, `Thumbs.db`). Make your first commit: `git add . && git commit -m "initial skeleton"`. Push to a new public GitHub repo.

**Watch out for.** Don't commit a `build/` directory by accident. Verify with `git status` after building that nothing inside `build/` shows up as untracked.

### Task 3: Write the top-level CMakeLists.txt

**Goal.** A CMakeLists.txt that compiles an empty `me_core` static library from your (currently empty) `.cpp` files.

**Approach.** Start with the CMakeLists.txt from Part V of the implementation brief. Strip it to the minimum: `cmake_minimum_required`, `project`, C++20 standard, the `add_library(me_core ...)` block, and `target_include_directories`. Skip the GoogleTest section for now — Task 4. Verify with `cmake -B build && cmake --build build` that it builds with no errors (even though the library has no real code).

**Watch out for.** Forgetting `target_include_directories(me_core PUBLIC include)` means later tests won't find your headers. Get it in from the start.

### Task 4: Pull in GoogleTest via FetchContent

**Goal.** GoogleTest available to the project, ready to use in tests.

**Approach.** Add the `FetchContent` block from the brief, pinned to a specific version tag (currently `v1.14.0` is safe; check [GoogleTest releases](https://github.com/google/googletest/releases) if you want the latest). Add `enable_testing()` and `add_subdirectory(tests)`. Create a `tests/CMakeLists.txt` that for now just contains a comment — Task 5 fills it in.

**Watch out for.** First `cmake -B build` will be slow because it downloads GoogleTest. Subsequent builds are fast.

### Task 5: Write the hello-world test and verify the pipeline

**Goal.** A trivial GoogleTest that runs as part of `ctest`, proving the test pipeline works end-to-end.

**Approach.** In `tests/CMakeLists.txt`, define an executable `me_tests` linking against `me_core`, `GTest::gtest`, and `GTest::gtest_main`. Use `include(GoogleTest)` and `gtest_discover_tests(me_tests)` to register tests with CTest. Create `tests/test_smoke.cpp` containing one trivial test: `TEST(Smoke, Passes) { EXPECT_EQ(1 + 1, 2); }`. Verify: `cmake --build build && ctest --test-dir build`. You should see one test, passing. Commit.

**Watch out for.** If `ctest` reports "no tests found," you probably forgot `gtest_discover_tests`. If the executable doesn't link, you forgot `GTest::gtest_main` (the library that provides `main()`).

---

## Milestone 0.2 — Domain types

### Task 6: Define the basic types

**Goal.** `types.h` contains `Price`, `Quantity`, `OrderId`, `Timestamp`, `Side`, `OrderType` — exactly as specified in the brief.

**Approach.** Open `include/me/types.h`. Add an `#include <cstdint>`, a header guard or `#pragma once`, a `namespace me { ... }` block, and the type aliases / enums from the brief. Keep enums as `enum class` with explicit underlying types (`std::uint8_t`). No logic yet — this is just declarations.

**Watch out for.** Don't use `int`, `unsigned`, `long`, or `size_t` for any of these. Use the fixed-width types. `size_t` is for sizes of things in memory; `Quantity` is a domain concept and should be its own type.

### Task 7: Define the Order struct

**Goal.** `order.h` contains the `Order` struct as a plain aggregate, no methods.

**Approach.** Open `include/me/order.h`. Include `types.h`. Inside `namespace me`, declare `struct Order` with the six fields from the brief (`id`, `side`, `type`, `price`, `quantity`, `timestamp`). Use the type aliases from `types.h` — `OrderId id`, not `uint64_t id`. Keep the struct plain (no constructors, no methods) for now; aggregate initialization is what we want.

**Watch out for.** Don't add a default constructor or member initializers yet. Plain aggregates are simpler. If later you find yourself wanting defaults, add them then.

### Task 8: Define the Trade struct

**Goal.** `trade.h` contains the `Trade` struct.

**Approach.** Same pattern as Task 7. Fields: `maker_id`, `taker_id`, `price`, `quantity`, `timestamp`. Add a comment above the struct explaining the maker/taker distinction (the brief covers this) — your future self in a code review will appreciate it.

### Task 9: Write basic construction tests

**Goal.** Tests that verify you can construct an Order and a Trade and read back their fields.

**Approach.** Create `tests/test_types.cpp`. Add it to `me_tests` in `tests/CMakeLists.txt` (add to the executable's source list). Write 2–3 tiny tests using aggregate initialization: `Order o{1, Side::Buy, OrderType::Limit, 10000, 50, 0}; EXPECT_EQ(o.id, 1u);`. Run `ctest`, see them pass. Commit.

**Watch out for.** Aggregate initialization is sensitive to field order. If you reorder fields in the struct later, these tests will silently break or produce wrong values. That's actually a feature — they'll catch you.

---

## Milestone 0.3 — OrderBook: non-crossing adds

### Task 10: Create the OrderBook class skeleton

**Goal.** `order_book.h` declares the `OrderBook` class with the data members and public API stubs from the brief, but no implementations.

**Approach.** Open `include/me/order_book.h`. Include `<map>`, `<list>`, `<unordered_map>`, `<vector>`, `<optional>`, plus your own headers. Inside `namespace me`, declare `class OrderBook`. In `private:`, declare the three members: `bids_` (a `std::map<Price, std::list<Order>, std::greater<Price>>`), `asks_` (same but `std::less<Price>`), and `index_` (a `std::unordered_map<OrderId, OrderLocation>`). Declare the nested `struct OrderLocation` with `side`, `price`, and `std::list<Order>::iterator iter`. In `public:`, declare (but don't define) `add_order`, `cancel_order`, `best_bid`, `best_ask`.

**Watch out for.** The `std::list<Order>::iterator` type inside `OrderLocation` is correct because all your FIFO queues are `std::list<Order>`. If you ever change that container type, this breaks. Add a comment noting the dependency.

### Task 11: Implement best_bid() and best_ask()

**Goal.** Both functions return `std::optional<Price>` correctly, with `std::nullopt` when the relevant side is empty.

**Approach.** Open `src/order_book.cpp`. Include `order_book.h`. Implement both functions following the exact code in the brief — three lines each. These are `const` member functions; mark them `const`.

**Watch out for.** Returning `bids_.begin()->first` only works because `bids_` is sorted by `std::greater<Price>`, so `begin()` points to the largest key. If you accidentally use `std::less<Price>` for bids, `best_bid()` will return the lowest bid — completely wrong, and a silent bug. Triple-check the comparator on `bids_`.

### Task 12: Implement add_order for non-crossing limit orders only

**Goal.** `add_order` accepts a limit order, inserts it into the correct side at the correct price level, updates the order-id index, and returns an empty trade vector. No matching logic yet — we'll add that in Milestone 0.4.

**Approach.** In `add_order`, branch on `order.side`. Pick `bids_` or `asks_`. Use `operator[]` on the map to get-or-create the price level. `push_back` the order. Capture the resulting iterator with `std::prev(level.end())`. Store an `OrderLocation` in `index_`. Return `std::vector<Trade>{}` (empty).

**Watch out for.** Don't worry about market orders or crossing here — assume the input is a non-crossing limit. We'll harden it in Milestone 0.4. But *do* handle the edge case of duplicate order id: check `index_.contains(order.id)` first and return empty if it's a duplicate, to avoid silent corruption. (Or use `index_.find`.)

### Task 13: Test non-crossing adds

**Goal.** Tests in `test_order_book.cpp` verify that adding limit orders updates the book and the best-price queries correctly.

**Approach.** Create `tests/test_order_book.cpp` and add it to the executable. Write tests for:
- Empty book: `best_bid()` and `best_ask()` are `std::nullopt`.
- Add one buy limit at 100: `best_bid()` is 100, `best_ask()` is still `nullopt`.
- Add one buy at 100, then one at 101: `best_bid()` is 101 (higher wins).
- Add one sell at 200, then one at 199: `best_ask()` is 199 (lower wins).
- Add a buy at 100 and a sell at 200: both sides report correctly. (No matching yet — these don't cross.)

Run tests. Fix bugs. Commit.

**Watch out for.** If you can't think of more tests for this stage, that's fine — six is enough. The matching tests in Milestone 0.4 will give the book a real workout.

---

## Milestone 0.4 — OrderBook: matching crossing orders

This is the densest milestone. Expect debugging. Take it one piece at a time.

### Task 14: Implement the matching loop core

**Goal.** Extend `add_order` so that if the incoming order crosses, it matches against the opposite side, generates trades, and reduces quantities.

**Approach.** Follow the pseudocode from the brief exactly. Structure the function as:
1. Compute `opposite_side` (pointer or reference to `bids_` or `asks_`).
2. Define a small lambda or local function that decides whether a given price level crosses the incoming order's limit.
3. The outer `while` loop iterates as long as quantity remains *and* the best opposite-side price exists *and* it crosses.
4. The inner `while` loop drains orders from the front of the FIFO at that price level.
5. After the loops, if the order was a limit and has quantity remaining, rest it in the book (the Task 12 code).

Implement the case where the incoming order matches against exactly *one* resting order and is fully consumed by it. Test that first before adding multi-level sweeps.

**Watch out for.** Remove price levels from the map when their FIFO becomes empty (`opposite_side.erase(price)`). If you don't, `best_ask()` will return phantom prices and your tests will fail mysteriously. This is the single most common bug in this code.

### Task 15: Handle multi-level sweeps

**Goal.** A large incoming order can consume multiple price levels in sequence.

**Approach.** This is mostly already handled by the `while` loop from Task 14 — when one price level empties, the loop should naturally proceed to the next. But you have to make sure you erase empty levels correctly so the "next-best price" lookup works. Test: pre-populate the book with several ask levels, submit a big buy that sweeps through them, verify all the expected trades are emitted in the right order.

**Watch out for.** Iterator invalidation. If you're holding a reference to the current price level's list while erasing the level from the map, that reference dies. Re-fetch on each loop iteration, or carefully sequence the erasure so it happens after you're done with the reference.

### Task 16: Handle partial fills

**Goal.** When the incoming order is partially consumed by a resting order (or vice versa), the leftover quantity is correctly retained.

**Approach.** Inside the inner loop, compute `traded_qty = std::min(incoming.quantity, resting.quantity)`. Subtract from both. If `resting.quantity == 0`, remove it from the FIFO and the index. If `incoming.quantity == 0`, break out. The resting order's quantity update has to actually persist — remember `incoming` is a local copy, but `resting` is a reference into the list. Make sure you're modifying the list's copy, not a temporary.

**Watch out for.** Forgetting that the *front* of the FIFO is being mutated in place. If you do `Order resting = queue.front();` (by value) and modify it, you've modified a copy, not the resting order. Use `Order& resting = queue.front();` — a reference.

### Task 17: Handle market orders

**Goal.** `add_order` correctly processes orders with `type == OrderType::Market` — they match against any available price, and unfilled remainder is discarded (not rested).

**Approach.** Adjust the "does this cross?" check: for market orders, every price crosses. Adjust the "rest the remainder" step at the end: only rest limit orders. If a market order can't be fully filled, the remainder simply vanishes.

**Watch out for.** Market orders are also subject to the "no opposite-side liquidity" case. If you submit a market buy and the asks side is empty, nothing happens — return empty trades, no remainder, no crash. Test this case explicitly.

### Task 18: Write the matching test suite

**Goal.** `test_order_book.cpp` covers all the matching scenarios from the brief's testing strategy section.

**Approach.** Write tests for each of the following, one at a time, fixing bugs as they appear:
- Crossing limit matches one resting order fully.
- Crossing limit matches one resting order partially (incoming fully filled, maker leftover).
- Crossing limit matches one resting order partially (maker fully filled, taker leftover that rests).
- Crossing limit sweeps multiple price levels.
- Crossing limit with quantity exceeding all opposite liquidity: takes everything and rests the rest at its limit.
- Market buy with no asks: returns empty trades, no remainder.
- Market buy fully filled by one level.
- Market buy that sweeps multiple levels.
- Time priority: two resting orders at the same price, the older one matches first.
- After a trade, `best_bid`/`best_ask` reflect the new state correctly.

Each test should set up the book deterministically, submit one order, assert on the returned trades and on `best_bid`/`best_ask`. Commit when all are green.

**Watch out for.** When you have a bug, the temptation is to "fix it" until the test passes. Resist this — understand *why* the bug happened before fixing. A bug fix you don't understand is a future bug waiting to recur.

---

## Milestone 0.5 — Cancel

### Task 19: Confirm the order-id index is being maintained

**Goal.** Audit the matching code from Milestone 0.4 to make sure that whenever an order rests, it goes into `index_`, and whenever an order is fully consumed (during matching), it's removed from `index_`.

**Approach.** Walk through your `add_order` code and verify both invariants. The resting case should already be there from Task 12. The "fully consumed during match" case might be missing — when you erase a resting order from a FIFO because its quantity hit zero, you also need `index_.erase(resting.id)`. Add a unit test that does several matches, then asks "is `index_` empty?" — it should be if all resting orders got consumed.

**Watch out for.** The index is invisible from outside the class, so a stale index produces bugs only later, in cancel paths. The randomized tests in Milestone 0.6 will catch any remaining issues, but it's faster to fix them now than to debug them later through a randomized stream.

### Task 20: Implement cancel_order

**Goal.** `cancel_order(id)` removes the order from the FIFO it lives in, removes the price level if it becomes empty, removes the index entry, and returns `true`. Unknown ids return `false`.

**Approach.** Follow the pseudocode from the brief. Look up in `index_`. If absent, return `false`. Otherwise, use the stored `side` to pick the right map, look up the price level, `erase` the list element using the stored iterator (O(1)), check if the list is empty and erase the price level if so, then erase the index entry. Return `true`.

**Watch out for.** Erasing the list element via the iterator works *only* because `std::list` iterators are stable. If you ever change the queue container, this code breaks silently — you'll be calling `erase` on an invalid iterator and getting undefined behavior. Add a comment about this dependency.

### Task 21: Write cancel tests

**Goal.** Tests covering cancel behavior in isolation and in combination with matching.

**Approach.** Write tests for:
- Cancel of an unknown id returns `false`.
- Cancel of a resting order returns `true` and removes it from `best_bid`/`best_ask` if it was the best.
- After cancel, a subsequent crossing order does not match against the canceled order.
- Cancel after partial fill: the maker's remaining quantity is what gets canceled, no zombie state.
- Cancel of the only order at a price level removes the level entirely (test by adding back at a different price and checking `best_bid`/`best_ask`).

Commit when green.

**Watch out for.** The "no zombie state" case is sneaky. If your code accidentally leaves a price level alive after canceling its last order, subsequent `best_bid`/`best_ask` calls return a phantom price. Test for this directly.

---

## Milestone 0.6 — Reference matcher and randomized tests

This is where you'll find bugs you didn't know you had. Budget for it.

### Task 22: Implement the reference matcher

**Goal.** A separate, deliberately-slow, deliberately-simple matcher implementing the same public API as `OrderBook`. Lives in `tests/reference_matcher.{h,cpp}`.

**Approach.** Use a single `std::vector<Order>` for each side. To find the best bid, do a linear scan. To match, sort the relevant side by price-then-timestamp, walk it from the front. To cancel, linear search by id. Don't optimize anything — the simplest correct code wins. The point is that this implementation is so transparent that you can review it and have *high confidence* it's correct.

**Watch out for.** Don't share any code between `OrderBook` and the reference matcher. They have to be independent implementations. Sharing helper functions defeats the purpose — a bug in the shared code wouldn't be caught by the cross-check.

### Task 23: Write the randomized cross-check harness

**Goal.** A test that generates a long random stream of operations, applies them to both implementations, and asserts identical behavior after every step.

**Approach.** Create `tests/test_randomized.cpp`. In a test, set up an `OrderBook` and a `ReferenceMatcher`. Seed a `std::mt19937` with a fixed seed (start with `42`). Loop for, say, 5000 iterations:
1. Decide an op: 60% add limit, 30% cancel (of a known resting id), 10% market.
2. Generate the op's parameters: random side, random price in a small range (say `[9990, 10010]`), random quantity, etc.
3. Apply to both implementations.
4. Assert the returned trades are identical (same length, same fields in same order).
5. Assert `best_bid()` and `best_ask()` are identical.

Wrap this in a parameterized test (or just a loop) over multiple seeds: 42, 1, 7, 100, 12345. If any seed fails, you've found a bug.

**Watch out for.** Tracking which order ids are currently resting (so you can pick a valid one to cancel) requires bookkeeping on the test side. The simplest approach: maintain a `std::vector<OrderId> resting` that you push to on add and remove from on cancel/match. Pick cancel targets uniformly from this vector. Update it consistently in both branches.

### Task 24: Fix every bug the randomized tests find

**Goal.** All five seeds run to completion with no assertion failures.

**Approach.** When a test fails, *do not* immediately start changing code. First, reduce the failing case to a minimal reproducer — find the smallest seed and operation count that still reproduces. Then trace through the operations one by one (your code should log each one in debug mode) and find the exact step where the two implementations diverge. That step's logic in `OrderBook` is wrong; fix it, then re-run.

**Watch out for.** Common bugs you'll find here: phantom price levels (Task 14 watch-out), stale index entries (Task 19 watch-out), partial-fill bookkeeping errors (Task 16 watch-out), and time-priority violations if you accidentally insert at the front of a FIFO instead of the back. Each of these is satisfying to find — they're real bugs that hand-written tests typically miss.

---

## Milestone 0.7 — MatchingEngine layer and v0.1 release

### Task 25: Implement the MatchingEngine wrapper

**Goal.** A thin class `MatchingEngine` that owns one `OrderBook` and forwards calls to it.

**Approach.** In `matching_engine.h/cpp`, declare a class with a single private `OrderBook book_` member. Public methods mirror `OrderBook`'s API: `add_order`, `cancel_order`, `best_bid`, `best_ask` — each forwards to `book_`. Yes, this is almost a no-op layer in Phase 0. It will earn its keep in Phase 3 when multiple instruments arrive.

**Watch out for.** Don't add features to `MatchingEngine` that don't belong there. Resist the urge to make it "more than a forwarder." Single-responsibility wins.

### Task 26: Write the README for v0.1

**Goal.** A clear, useful README that explains the project, how to build/test it, and the design at a high level.

**Approach.** Include sections: one-paragraph project description; quick start (clone, build, test commands); design overview (price-time priority, the two-level structure with sorted price levels and FIFO within, the order-id index for O(1) cancel); current status (Phase 0 done, Phase 1 next); design decisions worth highlighting (why `std::map`, why `std::list`, why integer prices). Keep it tight — half a page to a page of content. Add a "Development notes" subsection acknowledging AI assistance honestly.

**Watch out for.** Don't write the README as a personal blog post. It should read like a small project page at a serious company — clear, factual, lightly opinionated where you defend a decision. No emojis, no "🚀 awesome features 🚀" style.

### Task 27: Tag v0.1

**Goal.** A git tag marking the end of Phase 0.

**Approach.** `git tag -a v0.1 -m "Phase 0: correct matching engine"`. Push tags: `git push --tags`. Phase 0 is now a permanent reference point you can compare against.

---

## Milestone 1.1 — Order generator

### Task 28: Implement the synthetic order generator

**Goal.** A class `OrderGenerator` that produces an infinite stream of plausible orders, parameterized and reproducible.

**Approach.** Create `bench/order_generator.{h,cpp}`. The class holds a `std::mt19937` seeded in the constructor, and various distribution objects (for picking sides, prices, quantities, op types). Expose a method `Order next()` (or `Operation next()` if you want it to produce cancels too). Use the parameters from the brief: 60% add limit, 35% cancel, 5% market; prices Gaussian around mid; quantities uniform small range.

For cancels, the generator needs to know which order ids are currently resting in the book it's feeding. Either maintain that state inside the generator (it tracks what it's emitted and what's been canceled) or make `next()` take a small "set of cancellable ids" argument from outside.

**Watch out for.** A determinism trap: if your generator calls `rand()` (the C function) instead of using its own `std::mt19937` instance, it'll share state with anything else in the process that calls `rand()`, breaking reproducibility. Always use a local engine.

### Task 29: Test the generator in isolation

**Goal.** A unit test confirms the generator is reproducible (same seed → same stream) and produces a roughly expected distribution.

**Approach.** Write a test that creates two generators with seed=42, asks for 1000 operations each, and asserts they're identical. Then create one with seed=42 and one with seed=43, take 1000 each, and assert they're *different* (any difference is fine — just verifies the seed actually matters). Optional: count how many of each op type appear in 10000 samples, assert the ratios are within a few percent of the configured percentages.

**Watch out for.** "Roughly expected distribution" tests are fragile if you check too precisely. Use loose tolerances (e.g., "between 55% and 65% are adds" for a configured 60%) so the test doesn't flake.

---

## Milestone 1.2 — Google Benchmark integration

### Task 30: Pull in Google Benchmark and write the first benchmark

**Goal.** A benchmark executable `me_bench` that measures `add_order` against a steady-state book and reports throughput.

**Approach.** Add a `FetchContent_Declare` for `google/benchmark` in your top-level CMakeLists.txt (pin to `v1.8.3` or current latest). Set `BENCHMARK_ENABLE_TESTING OFF` before `FetchContent_MakeAvailable` to skip building Google Benchmark's own tests. Create `bench/CMakeLists.txt` defining `me_bench`, linking against `me_core`, `benchmark::benchmark`, and `benchmark::benchmark_main`. Create `bench/bench_main.cpp` containing the `BM_AddOrder_SteadyState` benchmark from the brief.

Build in Release mode: `cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`. Run `./build/bench/me_bench`. You should see Google Benchmark's formatted output with ns/iter and items/sec.

**Watch out for.** If you build in Debug or default mode, your numbers will be 10–50x slower than they should be. Always benchmark Release. Add a CMake warning if `CMAKE_BUILD_TYPE` is `Debug` and you're building the benchmark target — saves you from yourself.

---

## Milestone 1.3 — Latency distribution

### Task 31: Build the latency-distribution harness

**Goal.** A standalone executable that runs N operations, records every individual latency, and writes them to a CSV.

**Approach.** Create `bench/latency_dist.cpp`. The structure:
1. Construct `OrderBook`, `OrderGenerator`.
2. Warmup: 10,000 operations, untimed.
3. Pre-allocate a `std::vector<int64_t> latencies; latencies.reserve(1'000'000);`.
4. Loop 1,000,000 times: generate an order, `t0 = steady_clock::now()`, call `add_order`, `t1 = steady_clock::now()`, push `(t1 - t0).count()` into the vector.
5. After the loop, write all latencies to `bench/results/latency_raw.csv` (one number per line).

Add this as a second executable in `bench/CMakeLists.txt`. Build and run.

**Watch out for.** Pre-allocating the vector to its full size is critical. If it resizes mid-loop, you're measuring `std::vector::push_back`'s reallocation, not your engine. `.reserve` before the timed region.

### Task 32: Compute percentiles

**Goal.** From the CSV, compute p50, p99, p99.9 and produce a summary table.

**Approach.** Either write a tiny Python script (`bench/analyze.py`) using `numpy` to read the CSV and print percentiles, or write a small C++ program that does the same with `std::nth_element`. Python is faster to iterate on. The output should be a small markdown table you can paste into the README.

**Watch out for.** Percentiles from a stream are sensitive to the size of the sample. With 1M samples, p99.9 is the average of samples 999,000 and beyond — only ~1000 samples in that tail. That's enough to be meaningful but not enough to be super precise. Don't claim three significant figures for p99.9; one or two is honest.

---

## Milestone 1.4 — Baseline measurement

### Task 33: Run the baseline under controlled conditions

**Goal.** A set of numbers you can defend in interviews as your honest baseline.

**Approach.** Set up the environment:
- Linux machine (your laptop is fine; if you're on macOS or Windows, use a Linux VM or cloud instance).
- Disable CPU frequency scaling: `sudo cpupower frequency-set -g performance`.
- Close other applications; keep the system idle.
- Build in Release with `-O3`.
- Pin the benchmark to a single core: `taskset -c 2 ./build/bench/me_bench`.

Run the benchmark suite (Google Benchmark output + latency distribution CSV). Save the raw outputs into `bench/results/baseline/` — keep the actual files in the repo so the numbers in your README are reproducible from raw data.

**Watch out for.** Don't cherry-pick the best run. Run the suite three times back-to-back and report the *median* of the medians for each percentile. If the runs disagree dramatically, that's a sign of environmental noise — figure out the source before publishing.

### Task 34: Update the README with baseline results

**Goal.** The README has a "Benchmark Results — Baseline" table with hardware, methodology, and numbers.

**Approach.** Use the markdown template from the brief's Part VI. Fill in your actual hardware (`cat /proc/cpuinfo | grep "model name" | head -1` gets you the CPU), your compiler version (`g++ --version`), and the methodology (number of operations, seed, steady-state size). Drop the percentile table from Task 32 underneath.

**Watch out for.** Don't bury the numbers in prose — recruiters scanning the README want the table to pop. Put it in a section with a clear header and keep the surrounding text short.

### Task 35: Tag v0.2

**Goal.** Phase 1 complete, tagged.

**Approach.** `git tag -a v0.2 -m "Phase 1: benchmark harness and baseline"`. Push. The repo is now in a state you could honestly link from a resume or cover letter.

---

## What's after v0.2

Phase 2 (optimization) is the differentiator. Now that you have a correct engine and an honest baseline, profiling will tell you where to spend effort. The brief covers what to look for. But this is also a natural pause point — v0.2 is a respectable artifact on its own. If recruiting deadlines are close, prioritize applications over more code. If you have time, go for Phase 2.

A practical suggestion: before starting Phase 2, write yourself a short list of *predictions* — where do you think the hot spots are, what optimizations do you think will pay off, by how much? Then profile and see how right or wrong you were. Comparing your predictions to reality is one of the highest-leverage things you can do for developing engineering intuition, and it's the kind of self-aware learning that recruiters notice when you describe the project.

Good luck. Start with Task 1.
