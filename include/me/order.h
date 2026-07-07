#pragma once
#include "types.h"

namespace me {
    struct Order{
        OrderId id;
        Side side;
        OrderType type;
        Price price;
        Quantity quantity;
        Timestamp timestamp;
    };
} // namespace me
