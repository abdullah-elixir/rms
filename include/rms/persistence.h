#pragma once

#include "data_types.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <rocksdb/status.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/utilities/checkpoint.h>
#include <string>
#include <memory>
#include <vector>
#include <chrono>
#include <folly/container/F14Map.h>

namespace rms {

/**
 * @class PersistenceManager
 * @brief Manages persistence of risk management system state using RocksDB
 * 
 * This class handles:
 * - Position persistence with WAL
 * - Account/Instrument limits persistence
 * - Order/Trade audit logging
 * - State checkpointing
 * - Recovery from checkpoints
 */
class PersistenceManager {
public:
    PersistenceManager();
    ~PersistenceManager();

    /**
     * @brief Initialize the persistence layer
     * @param db_path Path to the RocksDB database file
     * @return true if initialization successful
     */
    bool initialize(const std::string& db_path);

    /**
     * @brief Create a checkpoint of the current state
     * @param checkpoint_path Path to store the checkpoint
     * @return true if checkpoint creation successful
     */
    bool createCheckpoint(const std::string& checkpoint_path);

    /**
     * @brief Restore state from a checkpoint
     * @param checkpoint_path Path to the checkpoint
     * @return true if restore successful
     */
    bool restoreFromCheckpoint(const std::string& checkpoint_path);

    /**
     * @brief Save current state to persistent storage
     * @return true if save successful
     */
    bool saveState();

    /**
     * @brief Load state from persistent storage
     * @return true if load successful
     */
    bool loadState();

    /**
     * @brief Save position data for a specific shard
     * @param shard_id The shard ID
     * @param positions The positions map to save
     */
    void savePositions(int shard_id, const folly::F14FastMap<uint32_t, Position>& positions);

    /**
     * @brief Load position data for a specific shard
     * @param shard_id The shard ID
     * @param positions The positions map to load into
     */
    void loadPositions(int shard_id, folly::F14FastMap<uint32_t, Position>& positions);

    /**
     * @brief Save account limits for a specific shard
     * @param shard_id The shard ID
     * @param limits The account limits to save
     */
    void saveAccountLimits(int shard_id, const AccountLimitsShard& limits);

    /**
     * @brief Load account limits for a specific shard
     * @param shard_id The shard ID
     * @param limits The account limits to load into
     */
    void loadAccountLimits(int shard_id, AccountLimitsShard& limits);

    /**
     * @brief Save instrument limits for a specific shard
     * @param shard_id The shard ID
     * @param limits The instrument limits to save
     */
    void saveInstrumentLimits(int shard_id, const InstrumentLimitsShard& limits);

    /**
     * @brief Load instrument limits for a specific shard
     * @param shard_id The shard ID
     * @param limits The instrument limits to load into
     */
    void loadInstrumentLimits(int shard_id, InstrumentLimitsShard& limits);

    /**
     * @brief Log an order for audit purposes
     * @param order The order to log
     */
    void logOrder(const Order& order);

    /**
     * @brief Log a trade execution for audit purposes
     * @param trade The trade to log
     */
    void logTrade(const TradeExecution& trade);

private:
    // RocksDB instance
    std::unique_ptr<rocksdb::DB> db_;
    rocksdb::Options options_;

    // Column family handles for different data types
    std::vector<rocksdb::ColumnFamilyHandle*> column_families_;
    rocksdb::ColumnFamilyHandle* positions_cf_;
    rocksdb::ColumnFamilyHandle* account_limits_cf_;
    rocksdb::ColumnFamilyHandle* instrument_limits_cf_;
    rocksdb::ColumnFamilyHandle* orders_cf_;
    rocksdb::ColumnFamilyHandle* trades_cf_;

    // WAL configuration
    bool wal_enabled_;
    std::string wal_path_;

    // Helper methods
    bool createColumnFamilies();
    bool setupWAL();
    bool savePositions(int shard_id);
    bool loadPositions(int shard_id);
    bool saveAccountLimits(int shard_id);
    bool loadAccountLimits(int shard_id);
    bool saveInstrumentLimits(int shard_id);
    bool loadInstrumentLimits(int shard_id);

    // Serialization helpers
    std::string serializePosition(const Position& pos);
    Position deserializePosition(const std::string& data);
    std::string serializeAccountLimits(const AccountLimits& limits);
    AccountLimits deserializeAccountLimits(const std::string& data);
    std::string serializeInstrumentLimits(const InstrumentLimits& limits);
    InstrumentLimits deserializeInstrumentLimits(const std::string& data);
    std::string serializeOrder(const Order& order);
    Order deserializeOrder(const std::string& data);
    std::string serializeTrade(const TradeExecution& trade);
    TradeExecution deserializeTrade(const std::string& data);

    // Key generation helpers
    std::string makePositionKey(int shard_id, uint32_t position_id);
    std::string makeAccountLimitsKey(int shard_id, uint32_t account_id);
    std::string makeInstrumentLimitsKey(int shard_id, uint32_t instrument_id);
    std::string makeOrderKey(uint64_t order_id);
    std::string makeTradeKey(uint64_t trade_id);
};

} // namespace rms 