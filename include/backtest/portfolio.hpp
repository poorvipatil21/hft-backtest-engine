#pragma once
#include "order.hpp"
#include <vector>
#include <cmath>
#include <algorithm>

namespace bt {

struct EquityPoint { Timestamp ts; double equity; };

// Tracks a single-instrument position and accumulates PnL from fills.
// Cash accounting is signed: buys reduce cash, sells increase it, and equity
// is marked to the prevailing mid.
class Portfolio {
public:
    explicit Portfolio(double starting_cash = 1'000'000.0, double tick_size = 0.01)
        : cash_(starting_cash), start_(starting_cash), tick_(tick_size) {}

    void on_fill(const Fill& f) {
        const double px  = static_cast<double>(f.price) * tick_;
        const double sq  = (f.side == Side::Buy ? +1.0 : -1.0) * static_cast<double>(f.qty);
        const double pos = static_cast<double>(position_);

        // Realize PnL on the portion that reduces the existing position.
        if ((pos > 0 && sq < 0) || (pos < 0 && sq > 0)) {
            double closing = std::min(std::abs(sq), std::abs(pos));
            double dir     = pos > 0 ? +1.0 : -1.0;
            realized_ += dir * (px - avg_px_) * closing;
        }

        const double new_pos = pos + sq;
        if ((pos >= 0 && sq > 0) || (pos <= 0 && sq < 0)) {
            // Increasing the position -> update the volume-weighted average price.
            double a = std::abs(pos), b = std::abs(sq);
            avg_px_ = (a + b) > 0 ? (avg_px_ * a + px * b) / (a + b) : px;
        } else if ((pos > 0 && new_pos < 0) || (pos < 0 && new_pos > 0)) {
            avg_px_ = px;  // position flipped through zero
        }

        cash_     -= sq * px;
        position_  = static_cast<Quantity>(std::llround(new_pos));
        ++fills_;
    }

    // Mark to market and append a point to the equity curve.
    void mark(Timestamp ts, double mid_px) {
        last_mark_ = mid_px;
        curve_.push_back({ts, equity(mid_px)});
    }

    double   equity(double mid_px) const { return cash_ + static_cast<double>(position_) * mid_px; }
    double   equity()   const { return cash_ + static_cast<double>(position_) * last_mark_; }
    double   pnl()      const { return equity() - start_; }
    double   realized() const { return realized_; }
    double   unrealized() const { return static_cast<double>(position_) * (last_mark_ - avg_px_); }
    double   cash()     const { return cash_; }
    Quantity position() const { return position_; }
    std::size_t fills() const { return fills_; }
    const std::vector<EquityPoint>& curve() const { return curve_; }

private:
    double      cash_, start_, tick_;
    Quantity    position_  = 0;
    double      avg_px_    = 0.0;
    double      last_mark_ = 0.0;
    double      realized_  = 0.0;
    std::size_t fills_     = 0;
    std::vector<EquityPoint> curve_;
};

} // namespace bt
