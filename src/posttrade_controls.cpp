//
// Created by muhammad-abdullah on 5/27/25.
//

// File: src/posttrade_controls.cpp
#include "posttrade_controls.h"
#include "data_types.h"
#include <algorithm>

void rms::PostTradeControls::onTrade(const TradeExecution &trade) {
    int shard = trade.account_id % NUM_SHARDS;
    auto &pos_map = position_store[shard];
    auto &pos = pos_map[trade.instrument_id];
    int64_t signed_qty = trade.is_buy ? trade.quantity : -trade.quantity;
    if ((pos.net_qty > 0 && signed_qty < 0) || (pos.net_qty < 0 && signed_qty > 0)) {
        int64_t close_qty = std::min<int64_t>(std::abs(pos.net_qty), std::abs(signed_qty));
        double exit_price = trade.price;
        double pnl = (pos.net_qty > 0)
            ? (exit_price - pos.avg_entry_price) * close_qty
            : (pos.avg_entry_price - exit_price) * close_qty;
        pos.realized_pnl += pnl;
        pos.net_qty += signed_qty;
        if (pos.net_qty == 0) pos.avg_entry_price = 0;
    } else {
        int64_t new_qty = pos.net_qty + signed_qty;
        if (new_qty != 0) {
            double new_avg = ((pos.avg_entry_price * pos.net_qty) + (trade.price * signed_qty)) / (double)new_qty;
            pos.avg_entry_price = new_avg;
        }
        pos.net_qty = new_qty;
    }
    double mark_price = trade.price; // stub
    pos.unrealized_pnl = (mark_price - pos.avg_entry_price) * pos.net_qty;
    double equity = pos.realized_pnl + pos.unrealized_pnl;
    pos.peak_equity = std::max(pos.peak_equity, equity);
    const auto &acct_lim = account_limits_shards[shard][trade.account_id % ACCOUNTS_PER_SHARD];
    //double used_margin = std::abs(pos.net_qty) * pos.avg_entry_price * acct_lim.init_margin_pct;
    //double maint_margin = std::abs(pos.net_qty) * pos.avg_entry_price * acct_lim.maint_margin_pct;
    //testing purpose
    double maint_margin = 5;
    if (equity < maint_margin) {
        // stub auto square-off
    }
    double drawdown_pct = (pos.peak_equity - equity) / pos.peak_equity;
    if (drawdown_pct > acct_lim.max_drawdown_pct) {
        // stub MDD liquidation
    }
}