#pragma once
#include "order.hpp"
#include "order_book.hpp"
#include <string>
#include <utility>

namespace bt {

// Level-1 view handed to the strategy on each market update.
struct MarketSnapshot {
    Timestamp ts       = 0;
    Price     best_bid = 0;
    Price     best_ask = 0;
    Quantity  bid_size = 0;
    Quantity  ask_size = 0;
    bool      has_bid  = false;
    bool      has_ask  = false;
    double    mid() const { return (static_cast<double>(best_bid) + static_cast<double>(best_ask)) / 2.0; }
    Price     spread() const { return best_ask - best_bid; }
};

// The action surface the engine exposes to a strategy. Submitting an order
// routes it through the latency model before it reaches the matching engine.
class OrderContext {
public:
    virtual ~OrderContext() = default;
    virtual OrderId submit(Side, OrderType, Price, Quantity) = 0;
    virtual void    cancel(OrderId) = 0;
    virtual Timestamp now() const = 0;
    virtual const OrderBook& book() const = 0;
};

// Base class for trading strategies. Override the callbacks of interest.
class Strategy {
public:
    virtual ~Strategy() = default;
    virtual void on_start(OrderContext&) {}
    virtual void on_market_data(const MarketSnapshot&, OrderContext&) = 0;
    virtual void on_fill(const Fill&, OrderContext&) {}
    virtual void on_stop(OrderContext&) {}
    const std::string& name() const { return name_; }

protected:
    explicit Strategy(std::string n) : name_(std::move(n)) {}
    std::string name_;
};

} // namespace bt
