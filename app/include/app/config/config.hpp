#pragma once

#include <cstdint>

namespace app {

/**
 * @brief Centralized configuration for the application.
 */
namespace config {

// Task priorities
constexpr uint32_t SHELL_TASK_PRIORITY = 1;
constexpr uint32_t ANALOG_ACQUISITION_TASK_PRIORITY = 3;
constexpr uint32_t SENSOR_RTT_TELEMETRY_TASK_PRIORITY = 1;

// Stack sizes
constexpr uint32_t SHELL_TASK_STACK_BYTES = 2048;
constexpr uint32_t ANALOG_ACQUISITION_TASK_STACK_BYTES = 2048;
constexpr uint32_t SENSOR_RTT_TELEMETRY_TASK_STACK_BYTES = 1024;

// Analog acquisition timestamping (TIM2 is configured to 1 MHz => 1 tick = 1 µs)
constexpr uint32_t ANALOG_ADC12_TICKS_PER_SEQUENCE_ESTIMATE = 13;
constexpr uint32_t ANALOG_ADC3_TICKS_PER_SEQUENCE_ESTIMATE = 13;

// Shell
constexpr uint32_t SHELL_TASK_IDLE_DELAY_MS = 10;

// RTT Telemetry
constexpr uint32_t RTT_TELEMETRY_SENSOR_CHANNEL = 1;
constexpr uint32_t RTT_TELEMETRY_SENSOR_BUFFER_SIZE = 1024;
constexpr uint32_t RTT_TELEMETRY_SENSOR_PERIOD_MS = 10;

}  // namespace config

}  // namespace app
