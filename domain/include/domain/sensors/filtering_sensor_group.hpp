#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "domain/sensors/sensor.hpp"

namespace domain::sensors {

template <typename FilterT>
class FilteringSensorGroup {
 public:
  FilteringSensorGroup(Sensor* const* sensors, FilterT* filters, std::size_t sensor_count) noexcept
      : sensors_(sensors), filters_(filters), sensor_count_(sensor_count) {
    if (sensors_ != nullptr) {
      for (std::size_t i = 0; i < sensor_count_; ++i) {
        assert(sensors_[i] != nullptr);
      }
    }
  }

  std::size_t count() const noexcept {
    return sensor_count_;
  }

  void UpdateAt(std::size_t index, std::uint16_t raw_value,
                std::uint32_t timestamp_ticks) noexcept {
    if (sensors_ == nullptr || filters_ == nullptr || index >= sensor_count_) {
      return;
    }
    Sensor* s = sensors_[index];
    if (s == nullptr) {
      return;
    }
    const std::uint16_t filtered_value = filters_[index].Apply(raw_value);
    s->Update(raw_value, filtered_value, timestamp_ticks);
  }

 private:
  Sensor* const* sensors_ = nullptr;
  FilterT* filters_ = nullptr;
  std::size_t sensor_count_ = 0;
};

}  // namespace domain::sensors
