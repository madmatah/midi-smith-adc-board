#include "bsp/adc/adc_dma.hpp"

#include "adc.h"
#include "bsp/memory_sections.hpp"
#include "stm32h7xx_hal.h"

namespace bsp::adc {
namespace {

alignas(32) BSP_AXI_SRAM_NOCACHE
    static std::uint32_t g_adc12_dma_buffer[AdcDma::kAdc12WordsPerBuffer];
alignas(32) BSP_D3_SRAM_NOCACHE
    static std::uint16_t g_adc3_dma_buffer[AdcDma::kAdc3HalfwordsPerBuffer];

static AdcDma* g_adc_dma = nullptr;

static bool PushDescriptor(os::Queue<AdcFrameDescriptor, 8>& queue, AdcGroup group,
                           std::uint8_t half, std::uint32_t sequence_id,
                           std::uint32_t timestamp_ticks, const void* data,
                           std::uint16_t element_count, std::uint8_t element_size_bytes) noexcept {
  const AdcFrameDescriptor desc{group, half,          sequence_id,       timestamp_ticks,
                                data,  element_count, element_size_bytes};
  return queue.SendFromIsr(desc);
}

}  // namespace

AdcDma::AdcDma(os::Queue<AdcFrameDescriptor, 8>& queue) noexcept : queue_(queue) {
  RegisterAdcDma(*this);
}

bool AdcDma::Start() noexcept {
  Stop();

  __disable_irq();
  running_ = true;
  __enable_irq();

  const HAL_StatusTypeDef adc12_status = HAL_ADCEx_MultiModeStart_DMA(
      &hadc1, reinterpret_cast<const std::uint32_t*>(g_adc12_dma_buffer), kAdc12WordsPerBuffer);
  if (adc12_status != HAL_OK) {
    Stop();
    return false;
  }

  const HAL_StatusTypeDef adc3_status = HAL_ADC_Start_DMA(
      &hadc3, reinterpret_cast<std::uint32_t*>(g_adc3_dma_buffer), kAdc3HalfwordsPerBuffer);
  if (adc3_status != HAL_OK) {
    Stop();
    return false;
  }

  return true;
}

void AdcDma::Stop() noexcept {
  __disable_irq();
  running_ = false;
  __enable_irq();

  (void) HAL_ADCEx_MultiModeStop_DMA(&hadc1);
  (void) HAL_ADC_Stop_DMA(&hadc3);
}

void AdcDma::HandleHalfComplete(AdcGroup group, std::uint32_t timestamp_ticks) noexcept {
  if (!running_) {
    return;
  }

  if (group == AdcGroup::kAdc12) {
    adc12_sequence_id_++;
    (void) PushDescriptor(queue_, group, 0, adc12_sequence_id_, timestamp_ticks, g_adc12_dma_buffer,
                          kAdc12WordsPerHalfBuffer, sizeof(std::uint32_t));
    return;
  }

  if (group == AdcGroup::kAdc3) {
    adc3_sequence_id_++;
    (void) PushDescriptor(queue_, group, 0, adc3_sequence_id_, timestamp_ticks, g_adc3_dma_buffer,
                          kAdc3HalfwordsPerHalfBuffer, sizeof(std::uint16_t));
  }
}

void AdcDma::HandleFullComplete(AdcGroup group, std::uint32_t timestamp_ticks) noexcept {
  if (!running_) {
    return;
  }

  if (group == AdcGroup::kAdc12) {
    adc12_sequence_id_++;
    (void) PushDescriptor(queue_, group, 1, adc12_sequence_id_, timestamp_ticks,
                          &g_adc12_dma_buffer[kAdc12WordsPerHalfBuffer], kAdc12WordsPerHalfBuffer,
                          sizeof(std::uint32_t));
    return;
  }

  if (group == AdcGroup::kAdc3) {
    adc3_sequence_id_++;
    (void) PushDescriptor(queue_, group, 1, adc3_sequence_id_, timestamp_ticks,
                          &g_adc3_dma_buffer[kAdc3HalfwordsPerHalfBuffer],
                          kAdc3HalfwordsPerHalfBuffer, sizeof(std::uint16_t));
  }
}

void RegisterAdcDma(AdcDma& adc_dma) noexcept {
  __disable_irq();
  g_adc_dma = &adc_dma;
  __enable_irq();
}

AdcDma* GetAdcDma() noexcept {
  return g_adc_dma;
}

}  // namespace bsp::adc
