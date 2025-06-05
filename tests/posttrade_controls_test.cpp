//
// Created by muhammad-abdullah on 5/27/25.
//
// File: tests/posttrade_controls_test.cpp
#include <gtest/gtest.h>
#include "posttrade_controls.h"
#include "data_types.h"
TEST(PostTradeControlsTest, BasicPnL) {
    rms::PostTradeControls pt;
    TradeExecution t{0,0,10,50,100.0,true};
    position_store[0][0] = Position();
    pt.onTrade(t);
    // After buy, unrealized_pnl = 0
    EXPECT_EQ(position_store[0][0].net_qty, 10);
}