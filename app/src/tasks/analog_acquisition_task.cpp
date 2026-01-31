#include "app/tasks/analog_acquisition_task.hpp"

#include <cstddef>
#include <cstdint>

#include "app/analog/acquisition_sequencer.hpp"
#include "app/analog/delay_requirements.hpp"
#include "app/config/config.hpp"
#include "os/queue_requirements.hpp"
#include "os/task.hpp"

namespace app::Tasks {
namespace {

inline std::uint32_t DeltaTicks(std::uint32_t now, std::uint32_t then) noexcept {
  return static_cast<std::uint32_t>(now - then);
}

inline std::uint32_t ComputeTicksPerSequence(std::uint32_t now_timestamp_ticks,
                                             std::uint32_t estimate_ticks_per_sequence,
                                             bool& has_prev_timestamp,
                                             std::uint32_t& prev_timestamp_ticks) noexcept {
  std::uint32_t ticks_per_sequence = estimate_ticks_per_sequence;
  if (has_prev_timestamp) {
    const std::uint32_t delta = DeltaTicks(now_timestamp_ticks, prev_timestamp_ticks);
    static_assert(bsp::adc::AdcDma::kSequencesPerHalfBuffer > 0u,
                  "kSequencesPerHalfBuffer must be > 0");
    ticks_per_sequence = delta / bsp::adc::AdcDma::kSequencesPerHalfBuffer;
  }
  prev_timestamp_ticks = now_timestamp_ticks;
  has_prev_timestamp = true;
  return ticks_per_sequence;
}

template <typename SampleT, std::size_t kRanksPerSequence, typename ApplyFn>
inline void ForEachTimestampedSequenceInHalfBuffer(const SampleT* data,
                                                   std::uint32_t end_timestamp_ticks,
                                                   std::uint32_t ticks_per_sequence,
                                                   ApplyFn apply) noexcept {
  const std::uint32_t first_ts = static_cast<std::uint32_t>(
      end_timestamp_ticks -
      static_cast<std::uint32_t>((bsp::adc::AdcDma::kSequencesPerHalfBuffer - 1u) *
                                 ticks_per_sequence));
  std::uint32_t ts = first_ts;
  const SampleT* seq_ptr = data;
  for (std::size_t seq = 0; seq < bsp::adc::AdcDma::kSequencesPerHalfBuffer; ++seq) {
    apply(seq_ptr, ts);
    seq_ptr += kRanksPerSequence;
    ts = static_cast<std::uint32_t>(ts + ticks_per_sequence);
  }
}

class TimestampCounterDelay final : public app::analog::DelayRequirements {
 public:
  explicit TimestampCounterDelay(
      app::time::TimestampCounterRequirements& timestamp_counter) noexcept
      : timestamp_counter_(timestamp_counter) {}

  void DelayUs(std::uint32_t delay_us) noexcept override {
    const std::uint32_t start = timestamp_counter_.NowTicks();
    for (;;) {
      const std::uint32_t now = timestamp_counter_.NowTicks();
      if (DeltaTicks(now, start) >= delay_us) {
        return;
      }
    }
  }

 private:
  app::time::TimestampCounterRequirements& timestamp_counter_;
};

bool ReceiveLatestCommand(os::Queue<app::analog::AcquisitionCommand, 4>& queue,
                          app::analog::AcquisitionCommand& cmd) noexcept {
  bool did_receive = false;
  app::analog::AcquisitionCommand tmp{};
  while (queue.Receive(tmp, os::kNoWait)) {
    cmd = tmp;
    did_receive = true;
  }
  return did_receive;
}

}  // namespace

AnalogAcquisitionTask::AnalogAcquisitionTask(
    os::Queue<bsp::adc::AdcFrameDescriptor, 8>& queue,
    os::Queue<app::analog::AcquisitionCommand, 4>& control_queue,
    bsp::GpioRequirements& tia_shutdown, bsp::adc::AdcDma& adc_dma,
    app::time::TimestampCounterRequirements& timestamp_counter,
    volatile app::analog::AcquisitionState& state, domain::sensors::SensorGroup& adc12_group,
    domain::sensors::SensorGroup& adc3_group) noexcept
    : queue_(queue),
      control_queue_(control_queue),
      tia_shutdown_(tia_shutdown),
      adc_dma_(adc_dma),
      timestamp_counter_(timestamp_counter),
      state_(state),
      adc12_group_(adc12_group),
      adc3_group_(adc3_group) {}

void AnalogAcquisitionTask::entry(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<AnalogAcquisitionTask*>(ctx)->run();
}

void AnalogAcquisitionTask::ResetDecodingState() noexcept {
  last_adc12_sequence_id_ = 0;
  last_adc3_sequence_id_ = 0;
  has_prev_adc12_timestamp_ = false;
  has_prev_adc3_timestamp_ = false;
  prev_adc12_timestamp_ = 0;
  prev_adc3_timestamp_ = 0;
}

void AnalogAcquisitionTask::DrainFrameQueue() noexcept {
  for (;;) {
    bsp::adc::AdcFrameDescriptor desc{};
    if (!queue_.Receive(desc, os::kNoWait)) {
      return;
    }
  }
}

void AnalogAcquisitionTask::EnterDisabledState() noexcept {
  tia_shutdown_.reset();
  adc_dma_.Stop();
  DrainFrameQueue();
  ResetDecodingState();
  state_ = app::analog::AcquisitionState::kDisabled;
}

void AnalogAcquisitionTask::HandleDisabledState(
    app::analog::AcquisitionSequencer& sequencer) noexcept {
  app::analog::AcquisitionCommand cmd{};
  if (!control_queue_.Receive(cmd, os::kWaitForever)) {
    return;
  }

  if (cmd == app::analog::AcquisitionCommand::kDisable) {
    EnterDisabledState();
    return;
  }

  if (cmd == app::analog::AcquisitionCommand::kEnable) {
    DrainFrameQueue();
    ResetDecodingState();
    const bool started = sequencer.Enable(70);
    if (!started) {
      EnterDisabledState();
      return;
    }
    state_ = app::analog::AcquisitionState::kEnabled;
  }
}

bool AnalogAcquisitionTask::TryHandleDisableRequestWhileEnabled() noexcept {
  app::analog::AcquisitionCommand cmd{};
  if (!ReceiveLatestCommand(control_queue_, cmd)) {
    return false;
  }
  if (cmd != app::analog::AcquisitionCommand::kDisable) {
    return false;
  }
  EnterDisabledState();
  return true;
}

void AnalogAcquisitionTask::ProcessAdc12Frame(const bsp::adc::AdcFrameDescriptor& desc) noexcept {
  const std::uint32_t ticks_per_sequence = ComputeTicksPerSequence(
      desc.timestamp_ticks, ::app::config::ANALOG_ADC12_TICKS_PER_SEQUENCE_ESTIMATE,
      has_prev_adc12_timestamp_, prev_adc12_timestamp_);

  const auto* words = static_cast<const std::uint32_t*>(desc.data);
  ForEachTimestampedSequenceInHalfBuffer<std::uint32_t, bsp::adc::AdcDma::kAdc12RanksPerSequence>(
      words, desc.timestamp_ticks, ticks_per_sequence,
      [this](const std::uint32_t* seq_ptr, std::uint32_t ts) noexcept {
        adc12_decoder_.ApplySequence(seq_ptr, bsp::adc::AdcDma::kAdc12RanksPerSequence,
                                     adc12_group_, ts);
      });
}

void AnalogAcquisitionTask::ProcessAdc3Frame(const bsp::adc::AdcFrameDescriptor& desc) noexcept {
  const std::uint32_t ticks_per_sequence = ComputeTicksPerSequence(
      desc.timestamp_ticks, ::app::config::ANALOG_ADC3_TICKS_PER_SEQUENCE_ESTIMATE,
      has_prev_adc3_timestamp_, prev_adc3_timestamp_);

  const auto* values = static_cast<const std::uint16_t*>(desc.data);
  ForEachTimestampedSequenceInHalfBuffer<std::uint16_t, bsp::adc::AdcDma::kAdc3RanksPerSequence>(
      values, desc.timestamp_ticks, ticks_per_sequence,
      [this](const std::uint16_t* seq_ptr, std::uint32_t ts) noexcept {
        adc3_decoder_.ApplySequence(seq_ptr, bsp::adc::AdcDma::kAdc3RanksPerSequence, adc3_group_,
                                    ts);
      });
}

void AnalogAcquisitionTask::ProcessFrame(const bsp::adc::AdcFrameDescriptor& desc) noexcept {
  if (desc.group == bsp::adc::AdcGroup::kAdc12) {
    ProcessAdc12Frame(desc);
    return;
  }
  if (desc.group == bsp::adc::AdcGroup::kAdc3) {
    ProcessAdc3Frame(desc);
  }
}

void AnalogAcquisitionTask::HandleEnabledState() noexcept {
  if (TryHandleDisableRequestWhileEnabled()) {
    return;
  }

  bsp::adc::AdcFrameDescriptor desc{};
  if (!queue_.Receive(desc, 1)) {
    return;
  }
  ProcessFrame(desc);
}

void AnalogAcquisitionTask::run() noexcept {
  timestamp_counter_.Start();

  EnterDisabledState();

  TimestampCounterDelay delay(timestamp_counter_);
  app::analog::AcquisitionSequencer sequencer(tia_shutdown_, delay, adc_dma_);

  for (;;) {
    if (state_ == app::analog::AcquisitionState::kDisabled) {
      HandleDisabledState(sequencer);
      continue;
    }
    HandleEnabledState();
  }
}

bool AnalogAcquisitionTask::start() noexcept {
  return os::Task::create("AnalogAcq", AnalogAcquisitionTask::entry, this,
                          ::app::config::ANALOG_ACQUISITION_TASK_STACK_BYTES,
                          ::app::config::ANALOG_ACQUISITION_TASK_PRIORITY);
}

}  // namespace app::Tasks
