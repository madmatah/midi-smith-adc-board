#pragma once

#include <cstdint>

#include "app/analog/acquisition_command.hpp"
#include "app/analog/acquisition_state.hpp"
#include "app/analog/adc12_frame_decoder.hpp"
#include "app/analog/adc3_frame_decoder.hpp"
#include "app/time/timestamp_counter_requirements.hpp"
#include "bsp/adc/adc_dma.hpp"
#include "bsp/gpio_requirements.hpp"
#include "domain/sensors/sensor_group.hpp"
#include "os/queue.hpp"

namespace app::analog {
class AcquisitionSequencer;
}  // namespace app::analog

namespace app::Tasks {

class AnalogAcquisitionTask {
 public:
  AnalogAcquisitionTask(os::Queue<bsp::adc::AdcFrameDescriptor, 8>& queue,
                        os::Queue<app::analog::AcquisitionCommand, 4>& control_queue,
                        bsp::GpioRequirements& tia_shutdown, bsp::adc::AdcDma& adc_dma,
                        app::time::TimestampCounterRequirements& timestamp_counter,
                        volatile app::analog::AcquisitionState& state,
                        domain::sensors::SensorGroup& adc12_group,
                        domain::sensors::SensorGroup& adc3_group) noexcept;

  bool start() noexcept;

 private:
  static void entry(void* ctx) noexcept;
  void run() noexcept;
  void ResetDecodingState() noexcept;
  void DrainFrameQueue() noexcept;
  void EnterDisabledState() noexcept;
  void HandleDisabledState(app::analog::AcquisitionSequencer& sequencer) noexcept;
  void HandleEnabledState() noexcept;
  bool TryHandleDisableRequestWhileEnabled() noexcept;
  void ProcessFrame(const bsp::adc::AdcFrameDescriptor& desc) noexcept;
  void ProcessAdc12Frame(const bsp::adc::AdcFrameDescriptor& desc) noexcept;
  void ProcessAdc3Frame(const bsp::adc::AdcFrameDescriptor& desc) noexcept;

  os::Queue<bsp::adc::AdcFrameDescriptor, 8>& queue_;
  os::Queue<app::analog::AcquisitionCommand, 4>& control_queue_;
  bsp::GpioRequirements& tia_shutdown_;
  bsp::adc::AdcDma& adc_dma_;
  app::time::TimestampCounterRequirements& timestamp_counter_;
  volatile app::analog::AcquisitionState& state_;
  domain::sensors::SensorGroup& adc12_group_;
  domain::sensors::SensorGroup& adc3_group_;

  app::analog::Adc12FrameDecoder adc12_decoder_{};
  app::analog::Adc3FrameDecoder adc3_decoder_{};

  std::uint32_t last_adc12_sequence_id_ = 0;
  std::uint32_t last_adc3_sequence_id_ = 0;

  bool has_prev_adc12_timestamp_ = false;
  bool has_prev_adc3_timestamp_ = false;
  std::uint32_t prev_adc12_timestamp_ = 0;
  std::uint32_t prev_adc3_timestamp_ = 0;
};

}  // namespace app::Tasks
