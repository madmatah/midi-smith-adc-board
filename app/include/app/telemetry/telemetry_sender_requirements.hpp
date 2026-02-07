#pragma once

#include <cstdint>
#include <span>

namespace app::telemetry {

class TelemetrySenderRequirements {
 public:
  virtual ~TelemetrySenderRequirements() = default;

  virtual void Send(std::span<const std::uint8_t> data) noexcept = 0;

  template <typename T>
  void Send(const T& value) noexcept {
    Send(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(&value), sizeof(T)));
  }
};

}  // namespace app::telemetry
