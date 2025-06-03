//
// Created by muhammad-abdullah on 5/28/25.
//
// File: tests/pretrade_checks_test.cpp
#include <gtest/gtest.h>
#include "pretrade_checks.h"
#include "data_types.h"

TEST(PreTradeChecksTest, MaxOrderQty) {
    rms::PreTradeChecks checker;
    Order o{0, 0, 50, 100.0};
    instrument_limits_shards[0][0].max_order_qty = 100;
    EXPECT_TRUE(checker.checkMaxOrderQty(o));
    std::cout<< checker.checkMaxOrderQty(o) << std::endl;
    o.quantity = 150;
    EXPECT_FALSE(checker.checkMaxOrderQty(o));
    std::cout<< checker.checkMaxOrderQty(o) << std::endl;
}
