#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "domain/sensors/sensor.hpp"

namespace domain::sensors {

namespace filtering_sensor_group_detail {

template <typename T, typename = void>
struct HasPush : std::false_type {};

template <typename T>
struct HasPush<T, std::void_t<decltype(std::declval<T&>().Push(std::declval<float>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct HasComputeOrRaw : std::false_type {};

template <typename T>
struct HasComputeOrRaw<
    T, std::void_t<decltype(std::declval<const T&>().ComputeOrRaw(std::declval<float>()))>>
    : std::true_type {};

}  // namespace filtering_sensor_group_detail

template <typename FilterT>
class FilteringSensorGroup {
 public:
  FilteringSensorGroup(Sensor* const* sensors, FilterT* filters, std::uint8_t* compute_phases,
                       std::size_t sensor_count, std::uint8_t compute_every) noexcept
      : sensors_(sensors),
        filters_(filters),
        compute_phases_(compute_phases),
        sensor_count_(sensor_count),
        compute_every_(compute_every) {
    if (sensors_ != nullptr) {
      for (std::size_t i = 0; i < sensor_count_; ++i) {
        assert(sensors_[i] != nullptr);
      }
    }
  }

  FilteringSensorGroup(Sensor* const* sensors, FilterT* filters, std::size_t sensor_count) noexcept
      : FilteringSensorGroup(sensors, filters, nullptr, sensor_count, 1) {}

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

    FilterT& filter = filters_[index];
    if constexpr (filtering_sensor_group_detail::HasPush<FilterT>::value &&
                  filtering_sensor_group_detail::HasComputeOrRaw<FilterT>::value) {
      filter.Push(static_cast<float>(raw_value));

      if (!ShouldComputeFilteredValue(index)) {
        s->UpdateRaw(raw_value, timestamp_ticks);
        return;
      }

      const float filtered_value = filter.ComputeOrRaw(static_cast<float>(raw_value));
      s->Update(raw_value, filtered_value, timestamp_ticks);
    } else {
      const float filtered_value = filter.Apply(static_cast<float>(raw_value));
      s->Update(raw_value, filtered_value, timestamp_ticks);
    }
  }

 private:
  bool ShouldComputeFilteredValue(std::size_t index) noexcept {
    if (compute_every_ <= 1 || compute_phases_ == nullptr) {
      return true;
    }

    std::uint8_t& phase = compute_phases_[index];
    const bool should_compute = (phase == 0u);

    const std::uint8_t next = static_cast<std::uint8_t>(phase + 1u);
    phase = (next >= compute_every_) ? 0u : next;
    return should_compute;
  }

  Sensor* const* sensors_ = nullptr;
  FilterT* filters_ = nullptr;
  std::uint8_t* compute_phases_ = nullptr;
  std::size_t sensor_count_ = 0;
  std::uint8_t compute_every_ = 1;
};

}  // namespace domain::sensors
