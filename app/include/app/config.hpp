#pragma once

#include <cstdint>

namespace app {

/**
 * @brief Centralized configuration for the application.
 */
namespace config {

// Task priorities
constexpr uint32_t TEST_TASK_PRIORITY = 2;

// Stack sizes
constexpr uint32_t TEST_TASK_STACK_BYTES = 1024;

// RTT Telemetry
constexpr uint32_t RTT_TELEMETRY_TEST_CHANNEL = 1;
constexpr uint32_t RTT_TELEMETRY_TEST_BUFFER_SIZE = 256;

}  // namespace config

}  // namespace app
