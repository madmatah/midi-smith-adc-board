#pragma once

#include <cstdint>

namespace domain::sensors {

enum class SensorRttMode : std::uint8_t { kRaw = 0, kProcessed = 1 };

}  // namespace domain::sensors
