#pragma once
#include <cstdint>

namespace ME{
    enum class Side { BID, ASK };
    enum class OrderType { LIMIT, MARKET };

    typedef std::int64_t Price;
        // Prices use int64_t to allow for a large range of prices, and to avoid floating point precision issues.
    typedef std::int64_t OrderId;
    typedef std::int64_t Quantity;
    typedef std::int64_t Timestamp;
  
    struct Order{
        OrderId id;
        Side side;
        OrderType type;
        Price price;
        Quantity quantity;
        // Order doesn't need a timestamp variable because the orderbook will maintain the time priority of orders 
        // in a price level by always appending to the end of the list, creating a FIFO.
    };

    struct Trade{
        OrderId maker_id;
        OrderId taker_id;
        Price price;
        Quantity quantity; 
        Timestamp timestamp;
    };
}
