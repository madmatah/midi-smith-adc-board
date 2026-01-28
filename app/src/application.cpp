#include "app/application.hpp"

#include <cstdint>
#include <new>

#include "app/config.hpp"
#include "app/tasks/shell_task.hpp"
#include "app/tasks/test_task.hpp"
#include "bsp/board.hpp"
#include "bsp/rtt_logger.hpp"
#include "bsp/rtt_telemetry_sender.hpp"
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
  console_stream.StartRxDma();

  // Test Task
  alignas(app::Tasks::TestTask) static std::uint8_t test_task_storage[sizeof(app::Tasks::TestTask)];
  static bool test_constructed = false;

  alignas(4) static std::uint8_t test_telemetry_buffer[app::config::RTT_TELEMETRY_TEST_BUFFER_SIZE];
  static bsp::RttTelemetrySender telemetry(app::config::RTT_TELEMETRY_TEST_CHANNEL, "TestTelemetry",
                                           test_telemetry_buffer, sizeof(test_telemetry_buffer));

  app::Tasks::TestTask* test_task_ptr = nullptr;
  if (!test_constructed) {
    test_task_ptr = new (test_task_storage) app::Tasks::TestTask(logger, telemetry);
    test_constructed = true;
  } else {
    test_task_ptr = reinterpret_cast<app::Tasks::TestTask*>(test_task_storage);
  }

  (void) test_task_ptr->start();

  // Shell Task
  static const shell::ShellConfig shell_config{"adc-board> "};
  alignas(
      app::Tasks::ShellTask) static std::uint8_t shell_task_storage[sizeof(app::Tasks::ShellTask)];
  static bool shell_constructed = false;

  app::Tasks::ShellTask* shell_task_ptr = nullptr;
  if (!shell_constructed) {
    shell_task_ptr = new (shell_task_storage) app::Tasks::ShellTask(console_stream, shell_config);
    shell_constructed = true;
  } else {
    shell_task_ptr = reinterpret_cast<app::Tasks::ShellTask*>(shell_task_storage);
  }

  (void) shell_task_ptr->start();
}

}  // namespace app
