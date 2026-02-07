#pragma once

#include <tuple>
#include <utility>

#include "domain/signal/processing_pipeline/detail.hpp"
#include "domain/signal/signal_processor_concepts.hpp"

namespace domain::signal::processing_pipeline {

template <SignalProcessor... StageTs>
class ContinuousPipeline {
 public:
  void Reset() noexcept {
    detail::ResetAll(stages_, std::make_index_sequence<sizeof...(StageTs)>{});
  }

  float Process(float input) noexcept {
    return detail::ProcessAll(stages_, input, std::make_index_sequence<sizeof...(StageTs)>{});
  }

 private:
  std::tuple<StageTs...> stages_{};
};

}  // namespace domain::signal::processing_pipeline
