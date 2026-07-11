#include "me/matching_engine.h"

std::vector<me::Trade> me::MatchingEngine::add_order(const Order& order) {
    return book_.add_order(order);
}

bool me::MatchingEngine::cancel_order(OrderId id) {
    return book_.cancel_order(id);
}

std::optional<me::Price> me::MatchingEngine::best_bid() const {
    return book_.best_bid();
}

std::optional<me::Price> me::MatchingEngine::best_ask() const {
    return book_.best_ask();
}