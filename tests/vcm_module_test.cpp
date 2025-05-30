//
// Created by muhammad-abdullah on 5/27/25.
//
// File: tests/vcm_module_test.cpp
#include <gtest/gtest.h>
#include "rms/vcm_module.h"

TEST(VCMModuleTest, SpreadCheck) {
    rms::VCMModule vcm;
    rms::Order o{0,0,10,50.0};
    EXPECT_TRUE(vcm.checkSpread(o));
}
