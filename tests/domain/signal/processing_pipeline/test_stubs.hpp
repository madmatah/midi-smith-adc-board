#pragma once

#include <cstdint>

namespace domain::signal::processing_pipeline::test {

class CounterStage {
 public:
  void Reset() noexcept {
    compute_count = 0;
    process_count = 0;
    push_count = 0;
    reset_count++;
  }

  void Push(float sample) noexcept {
    (void) sample;
    push_count++;
  }

  float ComputeOrRaw(float sample) const noexcept {
    compute_count++;
    return sample + 1.0f;
  }

  float Process(float sample) noexcept {
    process_count++;
    return sample + 1.0f;
  }

  static void ResetCounts() noexcept {
    compute_count = 0;
    process_count = 0;
    push_count = 0;
    reset_count = 0;
  }

  static inline std::uint32_t compute_count = 0;
  static inline std::uint32_t process_count = 0;
  static inline std::uint32_t push_count = 0;
  static inline std::uint32_t reset_count = 0;
};

class PlusTenStage {
 public:
  void Reset() noexcept {}

  float Process(float sample) noexcept {
    return sample + 10.0f;
  }
};

class TimesTwoStage {
 public:
  void Reset() noexcept {}

  float Process(float sample) noexcept {
    return sample * 2.0f;
  }
};

class PlusTenDecimatedStage {
 public:
  void Reset() noexcept {
    last_sample_ = 0.0f;
    has_sample_ = false;
  }

  void Push(float sample) noexcept {
    last_sample_ = sample;
    has_sample_ = true;
  }

  float ComputeOrRaw(float sample) const noexcept {
    return has_sample_ ? (last_sample_ + 10.0f) : (sample + 10.0f);
  }

 private:
  float last_sample_ = 0.0f;
  bool has_sample_ = false;
};

}  // namespace domain::signal::processing_pipeline::test
