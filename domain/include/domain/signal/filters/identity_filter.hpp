#pragma once

#include <cstdint>

namespace domain::signal::filters {

class IdentityFilter {
 public:
  std::uint16_t Apply(std::uint16_t sample) noexcept {
    return sample;
  }
};

}  // namespace domain::signal::filters
