#pragma once

namespace app::Logging {
class LoggerRequirements;
}

namespace app::telemetry {
class TelemetrySenderRequirements;
}

namespace app::Tasks {

class TestTask {
 public:
  TestTask(app::Logging::LoggerRequirements& logger,
           app::telemetry::TelemetrySenderRequirements& telemetry) noexcept;

  bool start() noexcept;

 private:
  static void entry(void* ctx) noexcept;
  void run() noexcept;

  app::Logging::LoggerRequirements& _logger;
  app::telemetry::TelemetrySenderRequirements& _telemetry;
};

}  // namespace app::Tasks
