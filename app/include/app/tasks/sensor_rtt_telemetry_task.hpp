#pragma once

#include <cstdint>

#include "app/analog/acquisition_state_requirements.hpp"
#include "app/telemetry/sensor_rtt_telemetry_command.hpp"
#include "app/telemetry/telemetry_sender_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "os/queue.hpp"

namespace app::Tasks {

class SensorRttTelemetryTask {
 public:
  SensorRttTelemetryTask(os::Queue<app::telemetry::SensorRttTelemetryCommand, 4>& control_queue,
                         domain::sensors::SensorRegistry& registry,
                         app::analog::AcquisitionStateRequirements& adc_state,
                         app::telemetry::TelemetrySenderRequirements& telemetry_sender,
                         volatile bool& enabled, volatile std::uint8_t& sensor_id,
                         volatile std::uint32_t& period_ms) noexcept;

  bool start() noexcept;

 private:
  static void entry(void* ctx) noexcept;
  void run() noexcept;

  void ApplyCommand(const app::telemetry::SensorRttTelemetryCommand& cmd) noexcept;

  os::Queue<app::telemetry::SensorRttTelemetryCommand, 4>& control_queue_;
  domain::sensors::SensorRegistry& registry_;
  app::analog::AcquisitionStateRequirements& adc_state_;
  app::telemetry::TelemetrySenderRequirements& telemetry_sender_;

  volatile bool& enabled_;
  volatile std::uint8_t& sensor_id_;
  volatile std::uint32_t& period_ms_;
};

}  // namespace app::Tasks
