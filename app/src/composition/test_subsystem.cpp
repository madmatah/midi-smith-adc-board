#include <cstdint>
#include <new>

#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/tasks/test_task.hpp"
#include "bsp/rtt_telemetry_sender.hpp"

namespace app::composition {

void CreateTestSubsystem(LoggingContext& logging) noexcept {
  alignas(4) static std::uint8_t test_telemetry_buffer[app::config::RTT_TELEMETRY_TEST_BUFFER_SIZE];
  static bsp::RttTelemetrySender telemetry(app::config::RTT_TELEMETRY_TEST_CHANNEL, "TestTelemetry",
                                           test_telemetry_buffer,
                                           static_cast<unsigned>(sizeof(test_telemetry_buffer)));

  alignas(app::Tasks::TestTask) static std::uint8_t test_task_storage[sizeof(app::Tasks::TestTask)];
  static bool test_constructed = false;

  app::Tasks::TestTask* test_task_ptr = nullptr;
  if (!test_constructed) {
    test_task_ptr = new (test_task_storage) app::Tasks::TestTask(logging.logger, telemetry);
    test_constructed = true;
  } else {
    test_task_ptr = reinterpret_cast<app::Tasks::TestTask*>(test_task_storage);
  }

  (void) test_task_ptr->start();
}

}  // namespace app::composition
