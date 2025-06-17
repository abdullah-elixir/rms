#pragma once

#include "logger.h"

namespace rms
{
    class LoggerWrapper
    {
    public:
        LoggerWrapper() = default;
        LoggerWrapper(uint8_t shard_count, const std::string &log_file)
        {
            for (uint8_t shard_id = 0; shard_id < shard_count; shard_id++)
            {
                std::string shard_log_file = log_file + "_shard_" + std::to_string(shard_id) + ".log";
                shard_loggers.emplace_back(std::make_unique<Logger>(shard_log_file));
            }
            // one more +1; for messaging class
            shard_loggers.emplace_back(std::make_unique<Logger>(log_file + "_messaging.log"));
        };
        ~LoggerWrapper()
        {
        }
        std::unique_ptr<Logger> &getLogger(uint8_t shard_id)
        {
            return shard_loggers[shard_id];
        }

        template <typename... Args>
        void trace(uint8_t shard_id, fmt::format_string<Args...> fmt, Args &&...args)
        {
            shard_loggers[shard_id]->trace(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void debug(uint8_t shard_id, fmt::format_string<Args...> fmt, Args &&...args)
        {
            shard_loggers[shard_id]->debug(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void info(uint8_t shard_id, fmt::format_string<Args...> fmt, Args &&...args)
        {
            shard_loggers[shard_id]->info(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void warn(uint8_t shard_id, fmt::format_string<Args...> fmt, Args &&...args)
        {
            shard_loggers[shard_id]->warn(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void error(uint8_t shard_id, fmt::format_string<Args...> fmt, Args &&...args)
        {
            shard_loggers[shard_id]->error(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        void critical(uint8_t shard_id, fmt::format_string<Args...> fmt, Args &&...args)
        {
            shard_loggers[shard_id]->critical(fmt, std::forward<Args>(args)...);
        }

        void flush(uint8_t shard_id) {
            shard_loggers[shard_id]->flush();
        }

    private:
        std::vector<std::unique_ptr<Logger>> shard_loggers;
    };
}