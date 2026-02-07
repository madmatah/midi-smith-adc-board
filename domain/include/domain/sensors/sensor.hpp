#pragma once

#include <cstdint>

namespace domain::sensors {

class Sensor {
 public:
  explicit Sensor(std::uint8_t id) noexcept : id_(id) {}

  std::uint8_t id() const noexcept {
    return id_;
  }

  std::uint16_t last_raw_value() const noexcept {
    return last_raw_value_;
  }

  std::uint16_t last_processed_value_int() const noexcept {
    return static_cast<std::uint16_t>(last_processed_value_);
  }

  float last_processed_value() const noexcept {
    return last_processed_value_;
  }

  std::uint32_t last_timestamp_ticks() const noexcept {
    return last_timestamp_ticks_;
  }

  void Update(std::uint16_t raw_value, std::uint32_t timestamp_ticks) noexcept {
    last_raw_value_ = raw_value;
    last_processed_value_ = static_cast<float>(raw_value);
    last_timestamp_ticks_ = timestamp_ticks;
  }

  void Update(std::uint16_t raw_value, float processed_value,
              std::uint32_t timestamp_ticks) noexcept {
    last_raw_value_ = raw_value;
    last_processed_value_ = processed_value;
    last_timestamp_ticks_ = timestamp_ticks;
  }

  void UpdateRaw(std::uint16_t raw_value, std::uint32_t timestamp_ticks) noexcept {
    last_raw_value_ = raw_value;
    last_timestamp_ticks_ = timestamp_ticks;
  }

 private:
  std::uint8_t id_ = 0;
  std::uint16_t last_raw_value_ = 0;
  float last_processed_value_ = 0.0f;
  std::uint32_t last_timestamp_ticks_ = 0;
};

}  // namespace domain::sensors
