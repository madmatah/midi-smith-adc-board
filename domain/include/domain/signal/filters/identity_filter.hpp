#pragma once

#include "domain/signal/signal_processor_concepts.hpp"

namespace domain::signal::filters {

class IdentityFilter {
 public:
  float Process(float sample) noexcept {
    return sample;
  }
  void Reset() noexcept {}
};

static_assert(domain::signal::is_signal_processor<IdentityFilter>::value,
              "IdentityFilter must satisfy SignalProcessor concept");

}  // namespace domain::signal::filters
