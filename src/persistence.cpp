//
// Created by muhammad-abdullah on 5/27/25.
//
// File: src/persistence.cpp
#include "persistence.h"
#include <folly/json.h>
#include <sstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/checkpoint.h>
#include <rocksdb/iterator.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/status.h>
#include <rocksdb/slice.h>

namespace rms {

PersistenceManager::PersistenceManager() 
    : positions_cf_(nullptr)
    , account_limits_cf_(nullptr)
    , instrument_limits_cf_(nullptr)
    , orders_cf_(nullptr)
    , trades_cf_(nullptr)
    , wal_enabled_(true) {
}

PersistenceManager::~PersistenceManager() {
    // Close all column family handles
    for (auto* cf : column_families_) {
        if (cf) {
            db_->DestroyColumnFamilyHandle(cf);
        }
    }
}

bool PersistenceManager::initialize(const std::string& db_path) {
    try {
        // Setup RocksDB options
        options_.create_if_missing = true;
        options_.create_missing_column_families = true;
        options_.max_background_jobs = 4;
        options_.max_write_buffer_number = 4;
        options_.write_buffer_size = 64 * 1024 * 1024; // 64MB
        options_.max_bytes_for_level_base = 256 * 1024 * 1024; // 256MB
        options_.target_file_size_base = 64 * 1024 * 1024; // 64MB
        options_.compression = rocksdb::kLZ4Compression;
        options_.bottommost_compression = rocksdb::kZSTD;
        options_.compression_opts.level = 4;
        options_.compression_opts.strategy = 0;
        options_.compression_opts.max_dict_bytes = 0;
        options_.compression_opts.zstd_max_train_bytes = 0;
        options_.compression_opts.parallel_threads = 0;
        options_.compression_opts.enabled = true;
        options_.compression_opts.max_dict_buffer_bytes = 0;

        // Enable WAL
        if (wal_enabled_) {
            wal_path_ = db_path + "/WAL";
            std::filesystem::create_directories(wal_path_);
            options_.WAL_ttl_seconds = 3600; // 1 hour
            options_.WAL_size_limit_MB = 1024; // 1GB
            options_.manual_wal_flush = true;
        }

        // Create column families
        std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "positions", rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "account_limits", rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "instrument_limits", rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "orders", rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "trades", rocksdb::ColumnFamilyOptions()));

        // Open DB with column families
        rocksdb::DB* db;
        auto status = rocksdb::DB::Open(options_, db_path, column_families, &column_families_, &db);
        if (!status.ok()) {
            std::cout << "Failed to open RocksDB: " << status.ToString() << std::endl;
            return false;
        }
        db_.reset(db);

        // Store column family handles
        positions_cf_ = column_families_[1];
        account_limits_cf_ = column_families_[2];
        instrument_limits_cf_ = column_families_[3];
        orders_cf_ = column_families_[4];
        trades_cf_ = column_families_[5];

        std::cout << "PersistenceManager initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "Failed to initialize PersistenceManager: " << e.what() << std::endl;
        return false;
    }
}

bool PersistenceManager::createCheckpoint(const std::string& checkpoint_path) {
    try {
        // Create checkpoint directory if it doesn't exist
        std::filesystem::create_directories(checkpoint_path);

        // Create checkpoint
        rocksdb::Checkpoint* checkpoint;
        auto status = rocksdb::Checkpoint::Create(db_.get(), &checkpoint);
        if (!status.ok()) {
            std::cout << "Failed to create checkpoint: " << status.ToString() << std::endl;
            return false;
        }

        // Export checkpoint
        status = checkpoint->CreateCheckpoint(checkpoint_path);
        delete checkpoint;

        if (!status.ok()) {
            std::cout << "Failed to export checkpoint: " << status.ToString() << std::endl;
            return false;
        }

        std::cout << "Checkpoint created successfully at " << checkpoint_path << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "Failed to create checkpoint: " << e.what() << std::endl;
        return false;
    }
}

bool PersistenceManager::restoreFromCheckpoint(const std::string& checkpoint_path) {
    try {
        // Close existing DB
        db_.reset();

        // Copy checkpoint to DB directory
        auto db_path = std::filesystem::path(checkpoint_path).parent_path();
        std::filesystem::remove_all(db_path);
        std::filesystem::create_directories(db_path);
        std::filesystem::copy(checkpoint_path, db_path, std::filesystem::copy_options::recursive);

        // Reopen DB
        return initialize(db_path.string());
    } catch (const std::exception& e) {
        std::cout << "Failed to restore from checkpoint: " << e.what() << std::endl;
        return false;
    }
}

void PersistenceManager::savePositions(int shard_id, const folly::F14FastMap<uint32_t, Position>& positions) {
    try {
        rocksdb::WriteBatch batch;

        for (const auto& [pos_id, position] : positions) {
            auto key = makePositionKey(shard_id, pos_id);
            auto value = serializePosition(position);
            batch.Put(positions_cf_, key, value);
        }

        rocksdb::WriteOptions write_options;
        write_options.sync = true;
        auto status = db_->Write(write_options, &batch);
        if (!status.ok()) {
            std::cout << "Failed to save positions for shard " << shard_id << ": " << status.ToString() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Failed to save positions for shard " << shard_id << ": " << e.what() << std::endl;
    }
}

void PersistenceManager::loadPositions(int shard_id, folly::F14FastMap<uint32_t, Position>& positions) {
    try {
        rocksdb::ReadOptions read_options;
        read_options.fill_cache = false;

        auto prefix = std::to_string(shard_id) + ":";
        auto it = db_->NewIterator(read_options, positions_cf_);
        for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
            auto pos_id = std::stoul(it->key().ToString().substr(prefix.length()));
            auto position = deserializePosition(it->value().ToString());
            positions[pos_id] = position;
        }
        delete it;
    } catch (const std::exception& e) {
        std::cout << "Failed to load positions for shard " << shard_id << ": " << e.what() << std::endl;
    }
}

void PersistenceManager::saveAccountLimits(int shard_id, const AccountLimitsShard& limits) {
    try {
        rocksdb::WriteBatch batch;

        for (size_t i = 0; i < limits.size(); ++i) {
            auto key = makeAccountLimitsKey(shard_id, i);
            auto value = serializeAccountLimits(limits[i]);
            batch.Put(account_limits_cf_, key, value);
        }

        rocksdb::WriteOptions write_options;
        write_options.sync = true;
        auto status = db_->Write(write_options, &batch);
        if (!status.ok()) {
            std::cout << "Failed to save account limits for shard " << shard_id << ": " << status.ToString() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Failed to save account limits for shard " << shard_id << ": " << e.what() << std::endl;
    }
}

void PersistenceManager::loadAccountLimits(int shard_id, AccountLimitsShard& limits) {
    try {
        rocksdb::ReadOptions read_options;
        read_options.fill_cache = false;

        auto prefix = std::to_string(shard_id) + ":acc:";
        auto it = db_->NewIterator(read_options, account_limits_cf_);
        for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
            auto account_id = std::stoul(it->key().ToString().substr(prefix.length()));
            auto account_limits = deserializeAccountLimits(it->value().ToString());
            limits[account_id] = account_limits;
        }
        delete it;
    } catch (const std::exception& e) {
        std::cout << "Failed to load account limits for shard " << shard_id << ": " << e.what() << std::endl;
    }
}

void PersistenceManager::saveInstrumentLimits(int shard_id, const InstrumentLimitsShard& limits) {
    try {
        rocksdb::WriteBatch batch;

        for (size_t i = 0; i < limits.size(); ++i) {
            auto key = makeInstrumentLimitsKey(shard_id, i);
            auto value = serializeInstrumentLimits(limits[i]);
            batch.Put(instrument_limits_cf_, key, value);
        }

        rocksdb::WriteOptions write_options;
        write_options.sync = true;
        auto status = db_->Write(write_options, &batch);
        if (!status.ok()) {
            std::cout << "Failed to save instrument limits for shard " << shard_id << ": " << status.ToString() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Failed to save instrument limits for shard " << shard_id << ": " << e.what() << std::endl;
    }
}

void PersistenceManager::loadInstrumentLimits(int shard_id, InstrumentLimitsShard& limits) {
    try {
        rocksdb::ReadOptions read_options;
        read_options.fill_cache = false;

        auto prefix = std::to_string(shard_id) + ":inst:";
        auto it = db_->NewIterator(read_options, instrument_limits_cf_);
        for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
            auto instrument_id = std::stoul(it->key().ToString().substr(prefix.length()));
            auto instrument_limits = deserializeInstrumentLimits(it->value().ToString());
            limits[instrument_id] = instrument_limits;
        }
        delete it;
    } catch (const std::exception& e) {
        std::cout << "Failed to load instrument limits for shard " << shard_id << ": " << e.what() << std::endl;
    }
}

void PersistenceManager::logOrder(const Order& order) {
    try {
        auto key = makeOrderKey(order.order_id);
        auto value = serializeOrder(order);

        rocksdb::WriteOptions write_options;
        write_options.sync = true;
        auto status = db_->Put(write_options, orders_cf_, key, value);
        if (!status.ok()) {
            std::cout << "Failed to log order " << order.order_id << ": " << status.ToString() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Failed to log order " << order.order_id << ": " << e.what() << std::endl;
    }
}

void PersistenceManager::logTrade(const TradeExecution& trade) {
    try {
        auto key = makeTradeKey(trade.trade_id);
        auto value = serializeTrade(trade);

        rocksdb::WriteOptions write_options;
        write_options.sync = true;
        auto status = db_->Put(write_options, trades_cf_, key, value);
        if (!status.ok()) {
            std::cout << "Failed to log trade " << trade.trade_id << ": " << status.ToString() << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Failed to log trade " << trade.trade_id << ": " << e.what() << std::endl;
    }
}

// Helper methods for key generation
std::string PersistenceManager::makePositionKey(int shard_id, uint32_t position_id) {
    return std::to_string(shard_id) + ":" + std::to_string(position_id);
}

std::string PersistenceManager::makeAccountLimitsKey(int shard_id, uint32_t account_id) {
    return std::to_string(shard_id) + ":acc:" + std::to_string(account_id);
}

std::string PersistenceManager::makeInstrumentLimitsKey(int shard_id, uint32_t instrument_id) {
    return std::to_string(shard_id) + ":inst:" + std::to_string(instrument_id);
}

std::string PersistenceManager::makeOrderKey(uint64_t order_id) {
    return "order:" + std::to_string(order_id);
}

std::string PersistenceManager::makeTradeKey(uint64_t trade_id) {
    return "trade:" + std::to_string(trade_id);
}

// Serialization methods
std::string PersistenceManager::serializePosition(const Position& pos) {
    folly::dynamic json = folly::dynamic::object
        ("net_qty", pos.net_qty)
        ("avg_entry_price", pos.avg_entry_price)
        ("realized_pnl", pos.realized_pnl)
        ("unrealized_pnl", pos.unrealized_pnl)
        ("peak_equity", pos.peak_equity);
    return folly::toJson(json);
}

Position PersistenceManager::deserializePosition(const std::string& data) {
    auto json = folly::parseJson(data);
    Position pos;
    pos.net_qty = json["net_qty"].asInt();
    pos.avg_entry_price = json["avg_entry_price"].asDouble();
    pos.realized_pnl = json["realized_pnl"].asDouble();
    pos.unrealized_pnl = json["unrealized_pnl"].asDouble();
    pos.peak_equity = json["peak_equity"].asDouble();
    return pos;
}

std::string PersistenceManager::serializeAccountLimits(const AccountLimits& limits) {
    folly::dynamic json = folly::dynamic::object
        ("max_order_rate_per_sec", limits.max_order_rate_per_sec)
        ("max_concurrent_orders", limits.max_concurrent_orders)
        ("max_leverage", limits.max_leverage)
        ("max_drawdown_pct", limits.max_drawdown_pct)
        ("kill_switch", limits.kill_switch);
    return folly::toJson(json);
}

AccountLimits PersistenceManager::deserializeAccountLimits(const std::string& data) {
    auto json = folly::parseJson(data);
    AccountLimits limits;
    limits.max_order_rate_per_sec = json["max_order_rate_per_sec"].asInt();
    limits.max_concurrent_orders = json["max_concurrent_orders"].asInt();
    limits.max_leverage = json["max_leverage"].asDouble();
    limits.max_drawdown_pct = json["max_drawdown_pct"].asDouble();
    limits.kill_switch = json["kill_switch"].asBool();
    return limits;
}

std::string PersistenceManager::serializeInstrumentLimits(const InstrumentLimits& limits) {
    folly::dynamic json = folly::dynamic::object
        ("max_order_qty", limits.max_order_qty)
        ("max_order_notional", limits.max_order_notional)
        ("price_tolerance_pct", limits.price_tolerance_pct)
        ("max_spread_ticks", limits.max_spread_ticks)
        ("init_margin_pct", limits.init_margin_pct)
        ("maint_margin_pct", limits.maint_margin_pct)
        ("max_daily_position", limits.max_daily_position);
    return folly::toJson(json);
}

InstrumentLimits PersistenceManager::deserializeInstrumentLimits(const std::string& data) {
    auto json = folly::parseJson(data);
    InstrumentLimits limits;
    limits.max_order_qty = json["max_order_qty"].asInt();
    limits.max_order_notional = json["max_order_notional"].asDouble();
    limits.price_tolerance_pct = json["price_tolerance_pct"].asDouble();
    limits.max_spread_ticks = json["max_spread_ticks"].asDouble();
    limits.init_margin_pct = json["init_margin_pct"].asDouble();
    limits.maint_margin_pct = json["maint_margin_pct"].asDouble();
    limits.max_daily_position = json["max_daily_position"].asInt();
    return limits;
}

std::string PersistenceManager::serializeOrder(const Order& order) {
    folly::dynamic json = folly::dynamic::object
        ("order_id", order.order_id)
        ("account_id", order.account_id)
        ("instrument_id", order.instrument_id)
        ("quantity", order.quantity)
        ("price", order.price)
        ("symbol", order.symbol)
        ("side", order.side);
    return folly::toJson(json);
}

Order PersistenceManager::deserializeOrder(const std::string& data) {
    auto json = folly::parseJson(data);
    Order order;
    order.order_id = json["order_id"].asInt();
    order.account_id = json["account_id"].asInt();
    order.instrument_id = json["instrument_id"].asInt();
    order.quantity = json["quantity"].asInt();
    order.price = json["price"].asDouble();
    strncpy(order.symbol, json["symbol"].asString().c_str(), sizeof(order.symbol) - 1);
    strncpy(order.side, json["side"].asString().c_str(), sizeof(order.side) - 1);
    return order;
}

std::string PersistenceManager::serializeTrade(const TradeExecution& trade) {
    folly::dynamic json = folly::dynamic::object
        ("order_id", trade.order_id)
        ("account_id", trade.account_id)
        ("instrument_id", trade.instrument_id)
        ("quantity", trade.quantity)
        ("price", trade.price)
        ("symbol", trade.symbol)
        ("is_buy", trade.is_buy)
        ("trade_id", trade.trade_id);
    return folly::toJson(json);
}

TradeExecution PersistenceManager::deserializeTrade(const std::string& data) {
    auto json = folly::parseJson(data);
    TradeExecution trade;
    trade.order_id = json["order_id"].asInt();
    trade.account_id = json["account_id"].asInt();
    trade.instrument_id = json["instrument_id"].asInt();
    trade.quantity = json["quantity"].asInt();
    trade.price = json["price"].asDouble();
    strncpy(trade.symbol, json["symbol"].asString().c_str(), sizeof(trade.symbol) - 1);
    trade.is_buy = json["is_buy"].asBool();
    trade.trade_id = json["trade_id"].asInt();
    return trade;
}

bool PersistenceManager::saveState() {
    try {
        // Flush all pending writes
        rocksdb::FlushOptions flush_options;
        flush_options.wait = true;
        auto status = db_->Flush(flush_options);
        if (!status.ok()) {
            std::cout << "Failed to flush DB: " << status.ToString() << std::endl;
            return false;
        }

        return true;
    } catch (const std::exception& e) {
        std::cout << "Failed to save state: " << e.what() << std::endl;
        return false;
    }
}

bool PersistenceManager::loadState() {
    try {
        // RocksDB automatically loads the latest state on startup
        // This method is a placeholder for any additional state loading logic
        return true;
    } catch (const std::exception& e) {
        std::cout << "Failed to load state: " << e.what() << std::endl;
        return false;
    }
}

bool PersistenceManager::createColumnFamilies() {
    try {
        // Create column families
        std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "positions", rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "account_limits", rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "instrument_limits", rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "orders", rocksdb::ColumnFamilyOptions()));
        column_families.push_back(rocksdb::ColumnFamilyDescriptor(
            "trades", rocksdb::ColumnFamilyOptions()));

        return true;
    } catch (const std::exception& e) {
        std::cout << "Failed to create column families: " << e.what() << std::endl;
        return false;
    }
}

bool PersistenceManager::setupWAL() {
    try {
        if (wal_enabled_) {
            options_.WAL_ttl_seconds = 3600; // 1 hour
            options_.WAL_size_limit_MB = 1024; // 1GB
            options_.manual_wal_flush = true;
            return true;
        }
        return true;
    } catch (const std::exception& e) {
        std::cout << "Failed to setup WAL: " << e.what() << std::endl;
        return false;
    }
}

} // namespace rms