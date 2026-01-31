#include "app/application.hpp"

#include "app/composition/subsystems.hpp"
#include "bsp/board.hpp"
#include "bsp/memory_sections.hpp"
#include "bsp/rtt_logger.hpp"
#include "bsp/serial/uart_stream.hpp"
#include "usart.h"

namespace app {

void Application::init() noexcept {
  bsp::Board::init();
}

void Application::create_tasks() noexcept {
  static bsp::RttLogger logger;

  // Console Stream (USART1)
  alignas(32) BSP_AXI_SRAM_NOCACHE static bsp::serial::UartStream<256, 1024> console_stream(huart1);
  (void) console_stream.StartRxDma();

  app::composition::ConsoleContext console{console_stream};

  app::composition::AdcControlContext adc_control = app::composition::CreateAnalogSubsystem();
  app::composition::AdcStateContext adc_state = app::composition::CreateAdcStateContext();
  app::composition::SensorsContext sensors = app::composition::CreateSensorsContext();

  app::composition::SensorRttTelemetryControlContext sensor_rtt =
      app::composition::CreateSensorRttTelemetrySubsystem(sensors, adc_state);
  app::composition::CreateShellSubsystem(console, adc_control, sensors, sensor_rtt);
}

}  // namespace app
