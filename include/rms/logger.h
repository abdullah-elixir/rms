#pragma once

#include <iostream>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string>
#include <memory>

namespace rms {

class Logger {
public:
    Logger() = default;
    Logger(std::string log_file = "risk_engine.log") {
        initialize(log_file);
    };
    void initialize(std::string log_file) {
        try {
            // Create console sink
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog::level::info);
            console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

            // Create file sink
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_file.data(), 1024*1024*5, 3);
            file_sink->set_level(spdlog::level::debug);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

            // Create logger with both sinks
            logger_ = std::make_shared<spdlog::logger>("risk_engine", 
                spdlog::sinks_init_list{console_sink, file_sink});
            logger_->set_level(spdlog::level::debug);
            
            spdlog::set_default_logger(logger_);
            spdlog::flush_on(spdlog::level::debug);
        } catch (const spdlog::spdlog_ex& ex) {
            std::cerr<<"Log initialization failed: " << ex.what() << std::endl;
        }
    }

    template<typename... Args>
    void trace(fmt::format_string<Args...> fmt, Args&&... args) {
        logger_->trace(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args) {
        logger_->debug(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args) {
        logger_->info(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args&&... args) {
        logger_->warn(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args) {
        logger_->error(fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void critical(fmt::format_string<Args...> fmt, Args&&... args) {
        logger_->critical(fmt, std::forward<Args>(args)...);
    }

    void flush() {
        logger_->flush();
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace rms

// Convenience macros
#define RMS_TRACE(...) rms::Logger::getInstance().trace(__VA_ARGS__)
#define RMS_DEBUG(...) rms::Logger::getInstance().debug(__VA_ARGS__)
#define RMS_INFO(...) rms::Logger::getInstance().info(__VA_ARGS__)
#define RMS_WARN(...) rms::Logger::getInstance().warn(__VA_ARGS__)
#define RMS_ERROR(...) rms::Logger::getInstance().error(__VA_ARGS__)
#define RMS_CRITICAL(...) rms::Logger::getInstance().critical(__VA_ARGS__) 