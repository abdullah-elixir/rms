//
// Created by muhammad-abdullah on 5/27/25.
//

// File: include/rms/utils/time_utils.hpp
#pragma once
#include <chrono>

namespace rms::utils {
    inline uint64_t nowMs() {
        return (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }
}