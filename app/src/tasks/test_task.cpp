#include "app/tasks/test_task.hpp"

#include "app/config.hpp"
#include "app/logging/logger_requirements.hpp"
#include "app/telemetry/telemetry_sender_requirements.hpp"
#include "app/version.hpp"
#include "os/clock.hpp"
#include "os/task.hpp"

namespace app::Tasks {

TestTask::TestTask(app::Logging::LoggerRequirements& logger,
                   app::telemetry::TelemetrySenderRequirements& telemetry) noexcept
    : _logger(logger), _telemetry(telemetry) {}

void TestTask::entry(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<TestTask*>(ctx)->run();
}

void TestTask::run() noexcept {
  bool is_active = false;
  _logger.infof("Firmware started: %s (%s) %s\n", version::kFullVersion.data(),
                version::kBuildType.data(), version::kCommitDate.data());

  for (;;) {
    _telemetry.Send(is_active ? 1u : 0u);
    _logger.infof("TestTask state changed: %s\n", is_active ? "active" : "inactive");
    os::Clock::delay_ms(1000);
    is_active = !is_active;
  }
}

bool TestTask::start() noexcept {
  return os::Task::create("TestTask", TestTask::entry, this, app::config::TEST_TASK_STACK_BYTES,
                          app::config::TEST_TASK_PRIORITY);
}

}  // namespace app::Tasks
