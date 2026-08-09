#pragma once
#include <cstdint>
#include <list>
#include <string>

namespace ME{
    enum class Side { BID, ASK };
    enum class OrderType { LIMIT, MARKET };

    typedef std::int64_t Price;
        // Prices use int64_t to avoid floating point precision issues.
    typedef std::int64_t ID;
    typedef std::int64_t Quantity;
    typedef std::int64_t Timestamp;
  
    struct Order{
        ID order_id;
        ID maker_id;
        std::string symbol;
        Side side;
        OrderType type;
        Price price;
        Quantity quantity;
        // Order doesn't need a timestamp variable because the orderbook will maintain the time priority of orders 
        // in a price level by always appending to the end of the list, creating a FIFO.
    };

    struct OrderLocation{
        Side side;
        Price price;
        std::list<Order>::iterator iterator; 
    };

    struct Trade{
        ID maker_id;
        ID taker_id;
        Price price;
        Quantity quantity; 
        Timestamp timestamp;
    };
}
