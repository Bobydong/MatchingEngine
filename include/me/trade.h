#pragma once
#include "types.h"

namespace me {
    struct Trade{
        OrderId maker_id;
        OrderId taker_id;
        Price price;
        Quantity quantity; 
        Timestamp timestamp;
    };
} // namespace me
