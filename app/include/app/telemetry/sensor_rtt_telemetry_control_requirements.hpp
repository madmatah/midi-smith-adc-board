#pragma once

#include <cstdint>

namespace app::telemetry {

struct SensorRttTelemetryStatus {
  bool enabled{false};
  std::uint8_t sensor_id{0};
  std::uint32_t period_ms{0};
};

class SensorRttTelemetryControlRequirements {
 public:
  virtual ~SensorRttTelemetryControlRequirements() = default;

  virtual bool RequestOff() noexcept = 0;
  virtual bool RequestObserve(std::uint8_t sensor_id, std::uint32_t period_ms) noexcept = 0;
  virtual SensorRttTelemetryStatus GetStatus() const noexcept = 0;
};

}  // namespace app::telemetry
