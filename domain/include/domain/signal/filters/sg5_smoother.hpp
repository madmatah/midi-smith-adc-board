#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace domain::signal::filters {

// Savitzky-Golay smoothing filter (with 5-point window)
class Sg5Smoother {
 public:
  static constexpr std::size_t kWindowSize = 5;

  void Reset() noexcept {
    history_.fill(0);
    next_index_ = 0;
    filled_ = 0;
  }

  std::uint16_t Apply(std::uint16_t sample) noexcept {
    Push(sample);
    return ComputeOrRaw(sample);
  }

  void Push(std::uint16_t sample) noexcept {
    history_[next_index_] = sample;
    next_index_ = NextIndex(next_index_);
    if (filled_ < kWindowSize) {
      ++filled_;
    }
  }

  std::uint16_t ComputeOrRaw(std::uint16_t raw_fallback) const noexcept {
    if (filled_ < kWindowSize) {
      return raw_fallback;
    }
    return Compute();
  }

 private:
  std::uint16_t Compute() const noexcept {
    const std::size_t i0 = next_index_;
    const std::size_t i1 = NextIndex(i0);
    const std::size_t i2 = NextIndex(i1);
    const std::size_t i3 = NextIndex(i2);
    const std::size_t i4 = NextIndex(i3);

    const std::uint16_t s0 = history_[i0];
    const std::uint16_t s1 = history_[i1];
    const std::uint16_t s2 = history_[i2];
    const std::uint16_t s3 = history_[i3];
    const std::uint16_t s4 = history_[i4];

    std::int32_t acc = 0;
    acc += 3 * static_cast<std::int32_t>(s0);
    acc += -5 * static_cast<std::int32_t>(s1);
    acc += -3 * static_cast<std::int32_t>(s2);
    acc += 9 * static_cast<std::int32_t>(s3);
    acc += 31 * static_cast<std::int32_t>(s4);

    const std::int32_t rounded = DivideRoundNearest(acc, 35);
    const std::int32_t clamped = std::clamp<std::int32_t>(rounded, 0, 65535);
    return static_cast<std::uint16_t>(clamped);
  }

  static constexpr std::size_t NextIndex(std::size_t index) noexcept {
    const std::size_t next = index + 1u;
    return (next >= kWindowSize) ? 0u : next;
  }

  static constexpr std::int32_t DivideRoundNearest(std::int32_t numerator,
                                                   std::int32_t denominator) noexcept {
    const std::int32_t half = denominator / 2;
    if (numerator >= 0) {
      return (numerator + half) / denominator;
    }
    return (numerator - half) / denominator;
  }

  std::array<std::uint16_t, kWindowSize> history_{};
  std::size_t next_index_ = 0;
  std::size_t filled_ = 0;
};

}  // namespace domain::signal::filters
