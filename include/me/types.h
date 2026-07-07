#pragma once

#include <cstdint>

namespace me {
    using Price     = std::int64_t;   // price in ticks; integer avoids float equality issues
    using Quantity  = std::int64_t;
    using OrderId   = std::uint64_t;
    using Timestamp = std::uint64_t;  // monotonic counter or nanoseconds since epoch

    enum class Side : std::uint8_t { Buy, Sell };
    enum class OrderType : std::uint8_t { Limit, Market };

} // namespace me
