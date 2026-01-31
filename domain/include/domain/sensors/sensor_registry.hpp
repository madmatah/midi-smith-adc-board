#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/sensors/sensor.hpp"

namespace domain::sensors {

class SensorRegistry {
 public:
  SensorRegistry(Sensor* sensors, std::size_t sensor_count) noexcept
      : sensors_(sensors), sensor_count_(sensor_count) {}

  Sensor* FindById(std::uint8_t id) noexcept {
    if (sensors_ == nullptr || id == 0) {
      return nullptr;
    }
    for (std::size_t i = 0; i < sensor_count_; ++i) {
      if (sensors_[i].id() == id) {
        return &sensors_[i];
      }
    }
    return nullptr;
  }

 private:
  Sensor* sensors_ = nullptr;
  std::size_t sensor_count_ = 0;
};

}  // namespace domain::sensors
