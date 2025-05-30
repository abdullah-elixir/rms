//
// Created by muhammad-abdullah on 5/27/25.
//
// File: tests/integration_test.cpp
#include <gtest/gtest.h>
#include "pretrade_checks.h"
#include "vcm_module.h"
#include "posttrade_controls.h"
#include "data_types.h"

TEST(IntegrationTest, OrderToTradeFlow) {
    rms::Order o{0,0,10,100.0};
    rms::PreTradeChecks pt;
    instrument_limits_shards[0][0].max_order_qty = 50;
    EXPECT_FALSE(pt.checkMaxOrderQty(o));
    o.quantity = 20;
    EXPECT_TRUE(pt.checkMaxOrderQty(o));
    rms::VCMModule vcm;
    EXPECT_TRUE(vcm.checkSpread(o));
    rms::TradeExecution t{0,0,20,100.0,true};
    position_store[0][0] = Position();
    rms::PostTradeControls ptc;
    ptc.onTrade(t);
    EXPECT_EQ(position_store[0][0].net_qty, 20);
}