// File: src/main.cpp
#include "risk_engine.h"
#include "messaging.h"
#include <csignal>
#include <iostream>

rms::RiskEngine *g_engine = nullptr;
void signalHandler(int signum) {
    if (g_engine) g_engine->stop();
    std::exit(signum);
}

int main(int argc, char **argv) {
    std::string config_path = "../config/risk_engine.yaml";
    rms::RiskEngine engine;
    g_engine = &engine;
    if (!engine.initialize(config_path)) return -1;
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    engine.start();
    // Keep running until signal
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}