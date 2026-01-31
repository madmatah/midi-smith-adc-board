#pragma once

#include <cstdint>

namespace app::telemetry {

enum class SensorRttTelemetryCommandKind : std::uint8_t {
  kOff = 0,
  kObserve = 1,
};

struct SensorRttTelemetryCommand {
  SensorRttTelemetryCommandKind kind{SensorRttTelemetryCommandKind::kOff};
  std::uint8_t sensor_id{0};
  std::uint32_t period_ms{0};
};

}  // namespace app::telemetry
