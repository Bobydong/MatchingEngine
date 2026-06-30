# PRD — C++ Limit Order Book & Matching Engine (+ Backtester Extension)

**One-liner:** A high-performance limit order book (LOB) and matching engine in modern C++ that ingests orders, matches them by price-time priority, and is benchmarked and optimized for low latency and high throughput. Phase 4 extends the engine into a market-replay backtester that runs a toy strategy against real historical order flow.

**Status:** Primary flagship (fully in your control — no external dependencies).

---

## 1. Why this project

**The problem it models.** Exchanges and trading firms maintain an order book and match incoming buy/sell orders in microseconds. Building one from scratch forces you to solve the core systems problems quant-dev roles care about: efficient data structures, a tight hot path, memory discipline, and measured performance. The backtester extension then puts you on the *participant* side of the same system — demonstrating you understand how strategies interact with a book.

**What it signals to recruiters.** C++ fluency, low-latency systems thinking, data-structure design, and — critically — *measure-then-optimize* discipline. This is the single most on-target artifact for quant-dev, and it doubles as a strong systems-SWE signal. With the Phase 4 backtester, it also gestures meaningfully toward quant research: you've shown you can reason about strategy evaluation, fill modeling, and adverse selection on real data.

**Interview story you're building toward (core):**
> "Built a limit order book matching engine in C++ that processes ~X orders/sec at a p99 latency of Y µs. Profiled the hot path and cut per-order latency Zx by replacing allocations with an object pool and restructuring the price-level data structures."

**Interview story (with Phase 4):**
> "...then turned the engine into a market-replay backtester: fed it real historical order flow from [LOBSTER / Binance], wrote a toy market-making strategy on top, and measured how it performed once you account for queue priority and adverse selection."

## 2. Goals & non-goals

**Goals**
- Correct price-time-priority matching for a single instrument.
- A benchmarking harness that measures throughput and latency.
- A documented optimization pass with before/after numbers.
- Clean, tested, well-documented public repo.
- *(Phase 4)* A market-replay backtester that runs a toy strategy through the engine against real historical data, with honest P&L reporting.

**Non-goals (resist these — they're scope creep)**
- A real exchange protocol stack, a GUI, or a live market data feed.
- Networking/distributed systems in the MVP (optional stretch only).
- Multiple asset classes or a "real" / profitable trading strategy. The Phase 4 strategy is a *demonstration* of how a strategy interacts with the book, not a money-making system.
- Anything that delays shipping Phases 0–2. The backtester does not exist until the core engine is correct, benchmarked, and optimized.

## 3. Scope by phase (each phase ends shippable)

**Phase 0 — Correct core (start here).** Single instrument. Support limit orders, market orders, and cancels. Maintain bids/asks; match incoming orders by price priority, then time (FIFO) at each price level; produce partial fills and trade events. Correctness over speed. Ship with a test suite.

**Phase 1 — Benchmark.** Build a harness that generates synthetic order flow and measures throughput (orders/sec) and latency percentiles (p50/p99). Record a baseline. *This is the milestone that turns the project from "a data structure" into "a performance project."*

**Phase 2 — Optimize.** Profile the hot path; remove allocations (object/memory pools), choose cache-friendly layouts, use intrusive lists for the FIFO queues, and add an order-id → location index for O(1) cancels. Re-benchmark and **document the speedups**. This is your differentiator — keep a before/after table.

**Phase 3 — Depth.** Add order modify, an L2 book-snapshot output, and (optionally) multiple instruments. Solidify the README with design rationale.

**Phase 4 — Market-replay backtester with toy strategy.** *(Only start after Phases 0–2 are rock solid and you have time before recruiting deadlines.)* Feed real historical order flow into the matching engine, drop a toy strategy on top, and measure how it performs. See Section 5b for design.

**Alternative Phase 4 extensions (only if backtester doesn't fit):** a multithreaded version with a lock-free input queue, or a minimal network input layer. Pick at most one — backtester is the recommended path.

## 4. Functional requirements

**Core engine (Phases 0–3)**
- **Order types:** limit, market, cancel; modify in Phase 3.
- **Matching:** strict price-time priority; partial fills; correct trade generation; correct book state after every event.
- **Order book ops:** add, cancel, match; O(1) best-bid/ask access; fast cancel by order id.
- **Output:** a stream of trade events and (Phase 3) L2 snapshots.
- **Inputs:** start with a simple in-process API or CSV order stream; no protocol parsing required.

**Backtester (Phase 4)**
- **Historical data ingestion:** parse one real-world order-flow dataset into the engine's order format. Recommended sources: LOBSTER (free Nasdaq samples, includes message + orderbook files) or a crypto exchange's public historical feed (Binance/Coinbase — easier to get bulk data for free).
- **Strategy interface:** a clean callback API where the strategy observes book state and trade events, and submits orders/cancels back into the engine. The strategy's orders are matched against the historical flow inside the same engine — preserving queue priority.
- **Toy strategy:** one deliberately simple market-making strategy. Example: quote ±N ticks around mid, cap inventory at ±M, cancel if mid moves too far. The point is to demonstrate the mechanism, not to make money.
- **P&L and metrics:** report total P&L, number of trades, average inventory, max drawdown, and (most importantly) a breakdown showing where you lose money — likely to adverse selection on directional moves. Honest reporting beats impressive-looking numbers.

## 5. Technical design — core engine

- **Language/build:** C++17 or C++20, CMake, compiled with `-O2/-O3`.
- **Core structures:** bids and asks as sorted price levels (`std::map` to start; consider a custom flat structure later); each price level is a FIFO of resting orders; a hash map from order id → node for O(1) cancel.
- **Hot-path discipline:** no heap allocation on the matching path; object pools for orders/nodes; intrusive linked lists; minimize branching and pointer chasing.
- **Testing:** GoogleTest or Catch2. Include a deliberately simple "reference" matcher and randomized/property tests that cross-check the fast engine against it — correctness you can trust is what lets you optimize aggressively.
- **Benchmarking:** a synthetic order-flow generator (configurable add/cancel/market mix); measure with steady-state warm runs; report p50/p99 and orders/sec.

## 5b. Technical design — backtester (Phase 4)

- **Architecture:** the matching engine is the simulator. Historical orders are fed in by timestamp; the strategy receives book/trade events through a callback API and submits its own orders back into the *same* engine. The engine doesn't know or care which orders are "historical" and which are "the strategy's" — they all match together with proper price-time priority. This is the key design choice that makes the backtest realistic.
- **Queue priority is real.** When the strategy posts a bid at $100.00, it joins the back of the queue at that price level. It only gets filled when all earlier orders at $100.00 are taken first, or when a seller comes through that crosses through to the strategy's price. This is the single biggest thing naive backtests get wrong, and the matching engine gives it to you for free.
- **Data format:** LOBSTER provides per-event "message" files (order add / cancel / execute) plus a matching order book state file. Crypto exchange data usually comes as either tick-level trades + L2 snapshots, or full order-by-order feeds (best for this use case). Pick one source, write a small adapter, and stick with it.
- **Strategy API:** define a `Strategy` interface with `on_book_update`, `on_trade`, `on_fill` callbacks. Keep it minimal — the goal is to show you can design a clean participant-side API, not to support arbitrary strategy logic.
- **Honesty in reporting.** The credibility move is to publish results that show the toy strategy *losing money to adverse selection* on real data and explain why. Interviewers see "I built a profitable strategy" claims constantly and almost all of them are bugs. A loss-making toy strategy with a correct explanation of *why* it loses is a far stronger signal than a flashy P&L curve.

## 6. Success criteria ("done")

**Core (required)**
- Matching correctness verified by unit + randomized tests.
- A benchmark report with baseline and optimized numbers, and a short write-up of *what* you changed and *why* it helped.
- Public GitHub repo (own repo, not a fork): clean README with design overview, the perf table, and build/run instructions.
- You can defend every design tradeoff out loud (why `std::map` vs alternatives, why an object pool, how cancel is O(1)).

**Backtester (if Phase 4 shipped)**
- Strategy runs end-to-end against at least one day of real historical data.
- README includes: the data source, the strategy logic in plain English, P&L results, and an honest analysis of why the strategy makes or (more likely) loses money.
- You can defend the queue-priority modeling and explain how it differs from a naive "I would have been filled at this price" backtest.

## 7. Risks & guardrails

- **Correctness before speed.** Don't optimize an engine you haven't proven correct.
- **Scope creep is the main threat.** Single instrument, no networking, no GUI until the core + benchmark + optimization story is complete. *The backtester is Phase 4 for a reason — do not start it before Phases 0–2 are shippable.*
- **Premature micro-optimization.** Get a baseline first; let the profiler tell you where to spend effort.
- **Backtester scope creep.** The strategy stays toy. As soon as you find yourself tuning parameters to "make it profitable," stop — you've drifted from quant-dev demonstration into quant-research speculation, and the latter requires a different skill set and much more rigor.
- **Time budget.** Phases 0–3 are realistically 2–3 months alongside coursework. Phase 4 adds another 1–2 months. If recruiting is close, ship Phases 0–2 and defer the backtester. A complete Phase 0–2 beats an incomplete Phase 0–4 every time.

## 8. Rough build order

M0 correct single-instrument book + tests → M1 benchmark harness + baseline → M2 optimize + document speedups → M3 modify / L2 output → **M4 data ingestion adapter → M5 strategy API + toy strategy → M6 backtest results + honest write-up**. Always keep `main` in a working, demoable state. If recruiting starts before M4, ship what you have — the core engine alone is the resume line.
