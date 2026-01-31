#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/sensors/sensor_group.hpp"

namespace app::analog {

class Adc12FrameDecoder {
 public:
  void ApplySequence(const std::uint32_t* packed_words, std::size_t word_count,
                     domain::sensors::SensorGroup& group, std::uint32_t timestamp_ticks) noexcept {
    if (packed_words == nullptr || word_count < 7 || group.count() < 14) {
      return;
    }

    for (std::size_t i = 0; i < 7; ++i) {
      const std::uint32_t w = packed_words[i];
      const std::uint16_t adc1 = static_cast<std::uint16_t>(w & 0xFFFFu);
      const std::uint16_t adc2 = static_cast<std::uint16_t>((w >> 16) & 0xFFFFu);
      group.UpdateAt((2u * i) + 0u, adc1, timestamp_ticks);
      group.UpdateAt((2u * i) + 1u, adc2, timestamp_ticks);
    }
  }
};

}  // namespace app::analog
