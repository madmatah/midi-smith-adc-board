#include <array>
#include <cstddef>
#include <cstdint>
#include <new>

#include "app/analog/queue_acquisition_control.hpp"
#include "app/composition/subsystems.hpp"
#include "app/config/sensors.hpp"
#include "app/config/sensors_validation.hpp"
#include "app/tasks/analog_acquisition_task.hpp"
#include "bsp/adc/adc_dma.hpp"
#include "bsp/pins.hpp"
#include "bsp/time/tim2_timestamp_counter.hpp"
#include "domain/sensors/filtering_sensor_group.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "domain/signal/filters/sg5_smoother.hpp"
#include "os/queue.hpp"

namespace app::composition {
namespace {

[[maybe_unused]] constexpr bool kConfigSensorsValidationIsUsed =
    app::config_sensors::validation::AreUnique(app::config_sensors::kSensorIds);

os::Queue<app::analog::AcquisitionCommand, 4>& AdcControlQueue() noexcept {
  static os::Queue<app::analog::AcquisitionCommand, 4> queue;
  return queue;
}

volatile app::analog::AcquisitionState& AdcState() noexcept {
  static volatile app::analog::AcquisitionState state = app::analog::AcquisitionState::kDisabled;
  return state;
}

app::analog::QueueAcquisitionControl& AdcControl() noexcept {
  static app::analog::QueueAcquisitionControl control(AdcControlQueue(), AdcState());
  return control;
}

domain::sensors::Sensor* SensorsArray() noexcept {
  alignas(domain::sensors::Sensor) static std::uint8_t
      sensors_storage[sizeof(domain::sensors::Sensor) * app::config_sensors::kSensorCount];
  static domain::sensors::Sensor* sensors =
      reinterpret_cast<domain::sensors::Sensor*>(sensors_storage);
  static bool sensors_constructed = false;
  if (!sensors_constructed) {
    for (std::size_t i = 0; i < app::config_sensors::kSensorCount; ++i) {
      new (&sensors[i]) domain::sensors::Sensor(app::config_sensors::kSensorIds[i]);
    }
    sensors_constructed = true;
  }
  return sensors;
}

domain::sensors::SensorRegistry& SensorsRegistry() noexcept {
  static domain::sensors::SensorRegistry registry(SensorsArray(),
                                                  app::config_sensors::kSensorCount);
  return registry;
}

using Filter = domain::signal::filters::Sg5Smoother;
using FilteredSensorGroup = domain::sensors::FilteringSensorGroup<Filter>;

void StartAnalogAcquisitionTask(FilteredSensorGroup& adc12_group,
                                FilteredSensorGroup& adc3_group) noexcept {
  static os::Queue<bsp::adc::AdcFrameDescriptor, 8> adc_frame_queue;
  static bsp::adc::AdcDma adc_dma(adc_frame_queue);
  static bsp::time::TimestampCounter timestamp_counter = bsp::time::CreateTim2TimestampCounter();

  alignas(app::Tasks::AnalogAcquisitionTask) static std::uint8_t
      analog_task_storage[sizeof(app::Tasks::AnalogAcquisitionTask)];
  static bool analog_constructed = false;
  app::Tasks::AnalogAcquisitionTask* analog_task_ptr = nullptr;
  if (!analog_constructed) {
    analog_task_ptr = new (analog_task_storage) app::Tasks::AnalogAcquisitionTask(
        adc_frame_queue, AdcControlQueue(), bsp::pins::TiaShutdown(), adc_dma, timestamp_counter,
        AdcState(), adc12_group, adc3_group);
    analog_constructed = true;
  } else {
    analog_task_ptr = reinterpret_cast<app::Tasks::AnalogAcquisitionTask*>(analog_task_storage);
  }

  (void) analog_task_ptr->start();
}

}  // namespace

AdcStateContext CreateAdcStateContext() noexcept {
  app::analog::AcquisitionStateRequirements& state = AdcControl();
  return AdcStateContext{state};
}

SensorsContext CreateSensorsContext() noexcept {
  return SensorsContext{SensorsRegistry()};
}

AdcControlContext CreateAnalogSubsystem() noexcept {
  static_assert(
      app::config_sensors::kAdc12ChannelCount == 2u * bsp::adc::AdcDma::kAdc12RanksPerSequence,
      "ADC12 mapping size must match AdcDma ranks");
  static_assert(app::config_sensors::kAdc3ChannelCount == bsp::adc::AdcDma::kAdc3RanksPerSequence,
                "ADC3 mapping size must match AdcDma ranks");

  static domain::sensors::Sensor* adc12_ptrs[app::config_sensors::kAdc12ChannelCount];
  static domain::sensors::Sensor* adc3_ptrs[app::config_sensors::kAdc3ChannelCount];
  static std::array<Filter, app::config_sensors::kAdc12ChannelCount> adc12_filters{};
  static std::array<Filter, app::config_sensors::kAdc3ChannelCount> adc3_filters{};

  domain::sensors::SensorRegistry& registry = SensorsRegistry();
  for (std::size_t i = 0; i < app::config_sensors::kAdc12ChannelCount; ++i) {
    adc12_ptrs[i] = registry.FindById(app::config_sensors::kAdc12SensorIds[i]);
  }
  for (std::size_t i = 0; i < app::config_sensors::kAdc3ChannelCount; ++i) {
    adc3_ptrs[i] = registry.FindById(app::config_sensors::kAdc3SensorIds[i]);
  }

  static FilteredSensorGroup adc12_group(adc12_ptrs, adc12_filters.data(),
                                         app::config_sensors::kAdc12ChannelCount);
  static FilteredSensorGroup adc3_group(adc3_ptrs, adc3_filters.data(),
                                        app::config_sensors::kAdc3ChannelCount);

  StartAnalogAcquisitionTask(adc12_group, adc3_group);
  return AdcControlContext{AdcControl()};
}

}  // namespace app::composition
