#pragma once

#include <cstdint>

#include "app/telemetry/sensor_rtt_telemetry_command.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "os/queue.hpp"

namespace app::telemetry {

class QueueSensorRttTelemetryControl final : public SensorRttTelemetryControlRequirements {
 public:
  QueueSensorRttTelemetryControl(os::Queue<SensorRttTelemetryCommand, 4>& queue,
                                 volatile bool& enabled, volatile std::uint8_t& sensor_id,
                                 volatile std::uint32_t& period_ms) noexcept
      : queue_(queue), enabled_(enabled), sensor_id_(sensor_id), period_ms_(period_ms) {}

  bool RequestOff() noexcept override {
    if (!enabled_) {
      return true;
    }
    SensorRttTelemetryCommand cmd{};
    cmd.kind = SensorRttTelemetryCommandKind::kOff;
    return queue_.Send(cmd, os::kNoWait);
  }

  bool RequestObserve(std::uint8_t sensor_id, std::uint32_t period_ms) noexcept override {
    SensorRttTelemetryCommand cmd{};
    cmd.kind = SensorRttTelemetryCommandKind::kObserve;
    cmd.sensor_id = sensor_id;
    cmd.period_ms = period_ms;
    return queue_.Send(cmd, os::kNoWait);
  }

  SensorRttTelemetryStatus GetStatus() const noexcept override {
    SensorRttTelemetryStatus s{};
    s.enabled = enabled_;
    s.sensor_id = sensor_id_;
    s.period_ms = period_ms_;
    return s;
  }

 private:
  os::Queue<SensorRttTelemetryCommand, 4>& queue_;
  volatile bool& enabled_;
  volatile std::uint8_t& sensor_id_;
  volatile std::uint32_t& period_ms_;
};

}  // namespace app::telemetry
