#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace rms {

class Metrics {
public:
    static Metrics& getInstance() {
        static Metrics instance;
        return instance;
    }

    // Operation counters
    void incrementOrderCount() { order_counter_.Increment(); }
    void incrementTradeCount() { trade_counter_.Increment(); }
    void incrementRejectedOrderCount() { rejected_order_counter_.Increment(); }
    void incrementLimitViolationCount() { limit_violation_counter_.Increment(); }

    // Latency measurements
    void recordOrderLatency(double seconds) { order_latency_.Observe(seconds); }
    void recordTradeLatency(double seconds) { trade_latency_.Observe(seconds); }

    // Position metrics
    void updatePositionCount(int count) { position_gauge_.Set(count); }
    void updateActiveAccountCount(int count) { active_account_gauge_.Set(count); }

    // Risk metrics
    void updateTotalExposure(double exposure) { total_exposure_gauge_.Set(exposure); }
    void updateMaxDrawdown(double drawdown) { max_drawdown_gauge_.Set(drawdown); }

    // Get metrics registry for Prometheus
    std::shared_ptr<prometheus::Registry> getRegistry() { return registry_; }

private:
    Metrics() {
        registry_ = std::make_shared<prometheus::Registry>();
        
        // Initialize counters
        order_counter_ = prometheus::Counter::Build()
            .Name("rms_orders_total")
            .Help("Total number of orders processed")
            .Register(*registry_);
            
        trade_counter_ = prometheus::Counter::Build()
            .Name("rms_trades_total")
            .Help("Total number of trades processed")
            .Register(*registry_);
            
        rejected_order_counter_ = prometheus::Counter::Build()
            .Name("rms_rejected_orders_total")
            .Help("Total number of rejected orders")
            .Register(*registry_);
            
        limit_violation_counter_ = prometheus::Counter::Build()
            .Name("rms_limit_violations_total")
            .Help("Total number of risk limit violations")
            .Register(*registry_);

        // Initialize histograms
        order_latency_ = prometheus::Histogram::Build()
            .Name("rms_order_latency_seconds")
            .Help("Order processing latency in seconds")
            .Register(*registry_);
            
        trade_latency_ = prometheus::Histogram::Build()
            .Name("rms_trade_latency_seconds")
            .Help("Trade processing latency in seconds")
            .Register(*registry_);

        // Initialize gauges
        position_gauge_ = prometheus::Gauge::Build()
            .Name("rms_positions_total")
            .Help("Total number of active positions")
            .Register(*registry_);
            
        active_account_gauge_ = prometheus::Gauge::Build()
            .Name("rms_active_accounts_total")
            .Help("Total number of active accounts")
            .Register(*registry_);
            
        total_exposure_gauge_ = prometheus::Gauge::Build()
            .Name("rms_total_exposure")
            .Help("Total exposure across all positions")
            .Register(*registry_);
            
        max_drawdown_gauge_ = prometheus::Gauge::Build()
            .Name("rms_max_drawdown")
            .Help("Maximum drawdown percentage")
            .Register(*registry_);
    }

    std::shared_ptr<prometheus::Registry> registry_;
    
    // Counters
    prometheus::Counter& order_counter_;
    prometheus::Counter& trade_counter_;
    prometheus::Counter& rejected_order_counter_;
    prometheus::Counter& limit_violation_counter_;

    // Histograms
    prometheus::Histogram& order_latency_;
    prometheus::Histogram& trade_latency_;

    // Gauges
    prometheus::Gauge& position_gauge_;
    prometheus::Gauge& active_account_gauge_;
    prometheus::Gauge& total_exposure_gauge_;
    prometheus::Gauge& max_drawdown_gauge_;
};

} // namespace rms 