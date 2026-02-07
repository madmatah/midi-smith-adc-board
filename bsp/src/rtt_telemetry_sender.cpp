#include "bsp/rtt_telemetry_sender.hpp"

#include "SEGGER_RTT.h"

namespace bsp {

RttTelemetrySender::RttTelemetrySender(unsigned channel, const char* name, void* buffer,
                                       unsigned size) noexcept
    : _channel(channel) {
  SEGGER_RTT_Init();
  (void) SEGGER_RTT_ConfigUpBuffer(_channel, name, buffer, size, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
}

void RttTelemetrySender::Send(std::span<const std::uint8_t> data) noexcept {
  (void) SEGGER_RTT_Write(_channel, data.data(), data.size());
}

}  // namespace bsp
