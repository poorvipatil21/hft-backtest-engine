// run_backtest — end-to-end backtest of a strategy on a synthetic L2 tape.
//
//   ./run_backtest [events] [strategy] [out_csv]
//     events    : number of market events to simulate (default 500000)
//     strategy  : "mm" (market maker, default) or "mr" (mean reversion)
//     out_csv   : equity-curve output path (default equity_curve.csv)
#include "backtest/backtester.hpp"
#include "backtest/analytics.hpp"
#include "market_maker.hpp"
#include "mean_reversion.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>

using namespace bt;

int main(int argc, char** argv) {
    std::size_t n_events = (argc > 1) ? std::strtoull(argv[1], nullptr, 10) : 500'000;
    std::string strat    = (argc > 2) ? argv[2] : "mm";
    std::string out_csv  = (argc > 3) ? argv[3] : "equity_curve.csv";

    std::printf("Generating synthetic L2 tape: %zu events...\n", n_events);
    auto feed = generate_synthetic_feed(n_events);

    Backtester::Config cfg;
    cfg.enable_friction = true;
    Backtester bt(cfg);

    std::unique_ptr<Strategy> strategy;
    if (strat == "mr") strategy = std::make_unique<MeanReversion>(50, 2.0, 100);
    else               strategy = std::make_unique<MarketMaker>(50, 1, 500);
    bt.set_strategy(strategy.get());

    std::printf("Running '%s' strategy with friction enabled...\n", strategy->name().c_str());
    bt.run(feed);

    BacktestReport rep = summarize(bt.portfolio(), bt.processed());

    std::printf("\n================ BACKTEST REPORT ================\n");
    std::printf("  Strategy          : %s\n",       strategy->name().c_str());
    std::printf("  Events processed  : %zu\n",      rep.events);
    std::printf("  Strategy fills    : %zu\n",      rep.fills);
    std::printf("  End position      : %lld\n",     (long long)rep.end_position);
    std::printf("  Final equity      : $%.2f\n",    rep.final_equity);
    std::printf("  Total PnL         : $%.2f\n",    rep.total_pnl);
    std::printf("  Return            : %.4f%%\n",   rep.return_pct);
    std::printf("  Sharpe (per-step) : %.3f\n",     rep.sharpe);
    std::printf("  Max drawdown      : %.4f%%\n",   rep.max_drawdown * 100.0);
    std::printf("=================================================\n");

    std::ofstream out(out_csv);
    out << "ts,equity\n";
    for (const auto& p : bt.portfolio().curve()) out << p.ts << ',' << p.equity << '\n';
    std::printf("Equity curve written to %s (%zu points)\n",
                out_csv.c_str(), bt.portfolio().curve().size());
    return 0;
}
