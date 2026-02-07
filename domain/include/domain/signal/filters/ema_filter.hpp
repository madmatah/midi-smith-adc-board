#pragma once

#include <cstdint>

#include "domain/signal/signal_processor_concepts.hpp"

namespace domain::signal::filters {

template <std::int32_t kAlphaNumerator, std::int32_t kAlphaDenominator>
class EmaFilterRatio {
  static_assert(kAlphaDenominator > 0, "kAlphaDenominator must be > 0");
  static_assert(kAlphaNumerator >= 0, "kAlphaNumerator must be >= 0");
  static_assert(kAlphaNumerator <= kAlphaDenominator,
                "kAlphaNumerator must be <= kAlphaDenominator");

  static constexpr float kAlpha =
      static_cast<float>(kAlphaNumerator) / static_cast<float>(kAlphaDenominator);

 public:
  void Reset() noexcept {
    has_value_ = false;
    value_ = 0.0f;
  }

  float Process(float sample) noexcept {
    Push(sample);
    return ComputeOrRaw(sample);
  }

  void Push(float sample) noexcept {
    if (!has_value_) {
      value_ = sample;
      has_value_ = true;
      return;
    }

    value_ = value_ + kAlpha * (sample - value_);
  }

  float ComputeOrRaw(float raw_fallback) const noexcept {
    if (!has_value_) {
      return raw_fallback;
    }
    return value_;
  }

 private:
  bool has_value_ = false;
  float value_ = 0.0f;
};

static_assert(domain::signal::is_signal_processor<EmaFilterRatio<1, 1>>::value,
              "EmaFilterRatio<1, 1> must satisfy SignalProcessor concept");
static_assert(domain::signal::is_decimation_compatible<EmaFilterRatio<1, 1>>::value,
              "EmaFilterRatio<1, 1> must satisfy DecimationCompatibleSignalProcessor concept");

}  // namespace domain::signal::filters
