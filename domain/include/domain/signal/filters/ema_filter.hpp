#pragma once

#include <algorithm>
#include <cstdint>

namespace domain::signal::filters {

template <std::int32_t kAlphaNumerator, std::int32_t kAlphaDenominator>
class EmaFilterRatio {
  static_assert(kAlphaDenominator > 0, "kAlphaDenominator must be > 0");
  static_assert(kAlphaNumerator >= 0, "kAlphaNumerator must be >= 0");
  static_assert(kAlphaNumerator <= kAlphaDenominator,
                "kAlphaNumerator must be <= kAlphaDenominator");

 public:
  void Reset() noexcept {
    has_value_ = false;
    value_ = 0;
  }

  std::uint16_t Apply(std::uint16_t sample) noexcept {
    Push(sample);
    return ComputeOrRaw(sample);
  }

  void Push(std::uint16_t sample) noexcept {
    if (!has_value_) {
      value_ = static_cast<std::int32_t>(sample);
      has_value_ = true;
      return;
    }

    const std::int32_t x = static_cast<std::int32_t>(sample);
    const std::int32_t diff = x - value_;
    const std::int32_t step = DivideRoundNearest(kAlphaNumerator * diff, kAlphaDenominator);
    const std::int32_t next = value_ + step;
    value_ = std::clamp<std::int32_t>(next, 0, 65535);
  }

  std::uint16_t ComputeOrRaw(std::uint16_t raw_fallback) const noexcept {
    if (!has_value_) {
      return raw_fallback;
    }
    return static_cast<std::uint16_t>(value_);
  }

 private:
  static constexpr std::int32_t DivideRoundNearest(std::int32_t numerator,
                                                   std::int32_t denominator) noexcept {
    const std::int32_t half = denominator / 2;
    if (numerator >= 0) {
      return (numerator + half) / denominator;
    }
    return (numerator - half) / denominator;
  }

  bool has_value_ = false;
  std::int32_t value_ = 0;
};

}  // namespace domain::signal::filters
