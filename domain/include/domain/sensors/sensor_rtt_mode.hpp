#pragma once

#include <cstdint>

namespace domain::sensors {

enum class SensorRttMode : std::uint8_t {
  kRaw = 0,
  kFiltered = 1,
  kBoth = 2,
};

}  // namespace domain::sensors
