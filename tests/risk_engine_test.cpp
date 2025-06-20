#include <gtest/gtest.h>
#include "rms/risk_engine.h"
#include "rms/persistence.h"
#include "rms/sharded_queue.h"
#include <thread>
#include <chrono>

class RiskEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
        engine = std::make_unique<rms::RiskEngine>();
        engine->initialize(4); // 4 shards for testing
    }

    void TearDown() override {
        engine.reset();
    }

    std::unique_ptr<rms::RiskEngine> engine;
};

// Basic Functionality Tests
TEST_F(RiskEngineTest, InitializationTest) {
    EXPECT_TRUE(engine->isInitialized());
    EXPECT_EQ(engine->getShardCount(), 4);
}

TEST_F(RiskEngineTest, PositionUpdateTest) {
    rms::Position pos;
    pos.account_id = 1;
    pos.instrument_id = 1;
    pos.net_qty = 100;
    pos.avg_entry_price = 100.0;
    
    EXPECT_TRUE(engine->updatePosition(1, pos));
    auto loaded_pos = engine->getPosition(1, 1);
    EXPECT_TRUE(loaded_pos.has_value());
    EXPECT_EQ(loaded_pos->net_qty, 100);
}

// Performance Tests
TEST_F(RiskEngineTest, HighThroughputTest) {
    const int NUM_OPERATIONS = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        rms::Position pos;
        pos.account_id = i % 1000;
        pos.instrument_id = i % 100;
        pos.net_qty = i;
        engine->updatePosition(pos.account_id % 4, pos);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should process at least 100k operations per second
    EXPECT_GT(NUM_OPERATIONS / (duration.count() / 1000.0), 100000);
}

// Concurrency Tests
TEST_F(RiskEngineTest, ConcurrentAccessTest) {
    const int NUM_THREADS = 8;
    const int OPERATIONS_PER_THREAD = 10000;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, t, OPERATIONS_PER_THREAD]() {
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                rms::Position pos;
                pos.account_id = (t * OPERATIONS_PER_THREAD + i) % 1000;
                pos.instrument_id = i % 100;
                pos.net_qty = i;
                engine->updatePosition(pos.account_id % 4, pos);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

// Recovery Tests
TEST_F(RiskEngineTest, RecoveryTest) {
    // Create some test data
    rms::Position pos;
    pos.account_id = 1;
    pos.instrument_id = 1;
    pos.net_qty = 100;
    engine->updatePosition(1, pos);
    
    // Simulate crash and recovery
    engine.reset();
    engine = std::make_unique<rms::RiskEngine>();
    engine->initialize(4);
    
    auto recovered_pos = engine->getPosition(1, 1);
    EXPECT_TRUE(recovered_pos.has_value());
    EXPECT_EQ(recovered_pos->net_qty, 100);
}

// Error Handling Tests
TEST_F(RiskEngineTest, InvalidInputTest) {
    rms::Position pos;
    pos.account_id = -1; // Invalid account ID
    EXPECT_FALSE(engine->updatePosition(0, pos));
    
    pos.account_id = 1;
    pos.instrument_id = -1; // Invalid instrument ID
    EXPECT_FALSE(engine->updatePosition(0, pos));
}

// Limit Tests
TEST_F(RiskEngineTest, RiskLimitTest) {
    rms::AccountLimits limits;
    limits.max_leverage = 2.0;
    limits.max_drawdown_pct = 0.1;
    engine->setAccountLimits(1, limits);
    
    rms::Position pos;
    pos.account_id = 1;
    pos.instrument_id = 1;
    pos.net_qty = 1000;
    pos.avg_entry_price = 100.0;
    
    EXPECT_FALSE(engine->updatePosition(1, pos)); // Should fail due to leverage limit
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 