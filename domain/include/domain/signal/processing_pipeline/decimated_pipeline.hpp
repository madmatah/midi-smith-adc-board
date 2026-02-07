#pragma once

#include <cstdint>
#include <tuple>
#include <utility>

#include "domain/signal/processing_pipeline/detail.hpp"
#include "domain/signal/signal_processor_concepts.hpp"

namespace domain::signal::processing_pipeline {

template <std::uint8_t kFactor, DecimationCompatibleSignalProcessor... StageTs>
class DecimatedPipeline {
 public:
  void Reset() noexcept {
    detail::ResetAll(stages_, std::make_index_sequence<sizeof...(StageTs)>{});
    phase_ = 0;
    last_computed_value_ = 0.0f;
    has_computed_ = false;
  }

  void Push(float input) noexcept {
    detail::PushAll(stages_, input, std::make_index_sequence<sizeof...(StageTs)>{});

    if (phase_ == 0) {
      last_computed_value_ =
          detail::ComputeAll(stages_, input, std::make_index_sequence<sizeof...(StageTs)>{});
      has_computed_ = true;
    }

    const std::uint8_t next = static_cast<std::uint8_t>(phase_ + 1u);
    phase_ = (next >= kFactor) ? 0u : next;
  }

  float Process(float input) noexcept {
    Push(input);
    return ComputeOrRaw(input);
  }

  float ComputeOrRaw(float raw_fallback) const noexcept {
    return has_computed_ ? last_computed_value_ : raw_fallback;
  }

 private:
  std::tuple<StageTs...> stages_{};
  float last_computed_value_ = 0.0f;
  bool has_computed_ = false;
  std::uint8_t phase_ = 0;
};

}  // namespace domain::signal::processing_pipeline
