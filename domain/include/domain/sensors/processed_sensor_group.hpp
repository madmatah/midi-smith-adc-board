#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "domain/sensors/sensor.hpp"
#include "domain/signal/signal_processor_concepts.hpp"

namespace domain::sensors {

template <typename ProcessorT>
class ProcessedSensorGroup {
 public:
  static_assert(domain::signal::is_signal_processor<ProcessorT>::value,
                "ProcessorT must satisfy SignalProcessor");

  ProcessedSensorGroup(Sensor* const* sensors, ProcessorT* processors,
                       std::size_t sensor_count) noexcept
      : sensors_(sensors), processors_(processors), sensor_count_(sensor_count) {
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
    if (sensors_ == nullptr || processors_ == nullptr || index >= sensor_count_) {
      return;
    }
    Sensor* s = sensors_[index];
    if (s == nullptr) {
      return;
    }

    ProcessorT& processor = processors_[index];
    const float raw_float = static_cast<float>(raw_value);

    const float processed_value = processor.Process(raw_float);

    s->Update(raw_value, processed_value, timestamp_ticks);
  }

 private:
  Sensor* const* sensors_ = nullptr;
  ProcessorT* processors_ = nullptr;
  std::size_t sensor_count_ = 0;
};

}  // namespace domain::sensors
