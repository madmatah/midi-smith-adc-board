#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "domain/sensors/sensor.hpp"

namespace domain::sensors {

class SensorGroup {
 public:
  SensorGroup(Sensor* const* sensors, std::size_t sensor_count) noexcept
      : sensors_(sensors), sensor_count_(sensor_count) {
    if (sensors != nullptr) {
      for (std::size_t i = 0; i < sensor_count; ++i) {
        assert(sensors[i] != nullptr);
      }
    }
  }

  std::size_t count() const noexcept {
    return sensor_count_;
  }

  void UpdateAt(std::size_t index, std::uint16_t raw_value,
                std::uint32_t timestamp_ticks) noexcept {
    if (sensors_ == nullptr || index >= sensor_count_) {
      return;
    }
    Sensor* s = sensors_[index];
    if (s == nullptr) {
      return;
    }
    s->Update(raw_value, timestamp_ticks);
  }

 private:
  Sensor* const* sensors_ = nullptr;
  std::size_t sensor_count_ = 0;
};

}  // namespace domain::sensors
