#pragma once

#include <cstdint>
#include <span>

#include "app/telemetry/telemetry_sender_requirements.hpp"

namespace bsp {

class RttTelemetrySender final : public app::telemetry::TelemetrySenderRequirements {
 public:
  RttTelemetrySender(unsigned channel, const char* name, void* buffer, unsigned size) noexcept;

  void Send(std::span<const std::uint8_t> data) noexcept override;

 private:
  unsigned _channel;
};

}  // namespace bsp
