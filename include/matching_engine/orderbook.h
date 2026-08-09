#include <map>
#include <list>


namespace ME {
    class orderbook {
        private:
            std::map<Price, std::list<Order>> bids_;
            std::map<Price, std::list<Order>, std::greater<Price>> asks_;
        public:
            bool add_order(const Order& order);
            bool remove_order(const Order& order);
            
    };

}
