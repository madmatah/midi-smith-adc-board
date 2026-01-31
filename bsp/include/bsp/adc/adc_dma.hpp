#pragma once

#include <cstddef>
#include <cstdint>

#include "app/analog/adc_dma_control_requirements.hpp"
#include "os/queue.hpp"

namespace bsp::adc {

enum class AdcGroup : std::uint8_t {
  kAdc12 = 0,
  kAdc3 = 1,
};

struct AdcFrameDescriptor {
  AdcGroup group;
  std::uint8_t half;
  std::uint32_t sequence_id;
  std::uint32_t timestamp_ticks;
  const void* data;
  std::uint16_t element_count;
  std::uint8_t element_size_bytes;
};

class AdcDma final : public app::analog::AdcDmaControlRequirements {
 public:
  static constexpr std::size_t kSequencesPerHalfBuffer = 8;

  static constexpr std::size_t kAdc12RanksPerSequence = 7;
  static constexpr std::size_t kAdc12WordsPerHalfBuffer =
      kSequencesPerHalfBuffer * kAdc12RanksPerSequence;
  static constexpr std::size_t kAdc12WordsPerBuffer = 2 * kAdc12WordsPerHalfBuffer;

  static constexpr std::size_t kAdc3RanksPerSequence = 8;
  static constexpr std::size_t kAdc3HalfwordsPerHalfBuffer =
      kSequencesPerHalfBuffer * kAdc3RanksPerSequence;
  static constexpr std::size_t kAdc3HalfwordsPerBuffer = 2 * kAdc3HalfwordsPerHalfBuffer;

  explicit AdcDma(os::Queue<AdcFrameDescriptor, 8>& queue) noexcept;

  bool Start() noexcept override;
  void Stop() noexcept override;

  void HandleHalfComplete(AdcGroup group, std::uint32_t timestamp_ticks) noexcept;
  void HandleFullComplete(AdcGroup group, std::uint32_t timestamp_ticks) noexcept;

 private:
  os::Queue<AdcFrameDescriptor, 8>& queue_;
  std::uint32_t adc12_sequence_id_ = 0;
  std::uint32_t adc3_sequence_id_ = 0;
  bool running_ = false;
};

void RegisterAdcDma(AdcDma& adc_dma) noexcept;
AdcDma* GetAdcDma() noexcept;

}  // namespace bsp::adc
