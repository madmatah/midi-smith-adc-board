#pragma once

#include "app/analog/acquisition_control_requirements.hpp"
#include "app/analog/acquisition_state_requirements.hpp"
#include "app/logging/logger_requirements.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "domain/io/stream_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"

namespace app::composition {

struct LoggingContext {
  app::Logging::LoggerRequirements& logger;
};

struct ConsoleContext {
  domain::io::StreamRequirements& stream;
};

struct AdcControlContext {
  app::analog::AcquisitionControlRequirements& control;
};

struct AdcStateContext {
  app::analog::AcquisitionStateRequirements& state;
};

struct SensorsContext {
  domain::sensors::SensorRegistry& registry;
};

struct SensorRttTelemetryControlContext {
  app::telemetry::SensorRttTelemetryControlRequirements& control;
};

AdcControlContext CreateAnalogSubsystem() noexcept;
AdcStateContext CreateAdcStateContext() noexcept;
SensorsContext CreateSensorsContext() noexcept;


SensorRttTelemetryControlContext CreateSensorRttTelemetrySubsystem(
    SensorsContext& sensors, AdcStateContext& adc_state) noexcept;
void CreateTestSubsystem(LoggingContext& logging) noexcept;
void CreateShellSubsystem(ConsoleContext& console, AdcControlContext& adc_control,
                          SensorsContext& sensors,
                          SensorRttTelemetryControlContext& sensor_rtt) noexcept;

}  // namespace app::composition
