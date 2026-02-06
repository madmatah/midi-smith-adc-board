#pragma once

#include <cstdint>

namespace domain::signal::filters {

class IdentityFilter {
 public:
  float Apply(float sample) noexcept {
    return sample;
  }
};

}  // namespace domain::signal::filters
