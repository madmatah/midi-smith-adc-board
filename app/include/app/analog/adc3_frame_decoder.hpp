#pragma once

#include <cstddef>
#include <cstdint>

namespace app::analog {

class Adc3FrameDecoder {
 public:
  template <typename GroupT>
  void ApplySequence(const std::uint16_t* values, std::size_t value_count, GroupT& group,
                     std::uint32_t timestamp_ticks) noexcept {
    if (values == nullptr || value_count < 8 || group.count() < 8) {
      return;
    }

    for (std::size_t i = 0; i < 8; ++i) {
      group.UpdateAt(i, values[i], timestamp_ticks);
    }
  }
};

}  // namespace app::analog
