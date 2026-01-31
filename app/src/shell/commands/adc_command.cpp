#include "app/shell/commands/adc_command.hpp"

#include <string_view>

namespace app::shell::commands {
namespace {

std::string_view Arg(int argc, char** argv, int index) noexcept {
  if (argv == nullptr) {
    return {};
  }
  if (index < 0 || index >= argc) {
    return {};
  }
  if (argv[index] == nullptr) {
    return {};
  }
  return std::string_view(argv[index]);
}

void WriteUsage(domain::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: adc on|off|status\r\n");
}

}  // namespace

void AdcCommand::Run(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept {
  const std::string_view op = Arg(argc, argv, 1);
  if (op.empty()) {
    WriteUsage(out);
    return;
  }

  if (op == "on") {
    if (!control_.RequestEnable()) {
      out.Write("error: enable request rejected\r\n");
      return;
    }
    out.Write("ok\r\n");
    return;
  }

  if (op == "off") {
    if (!control_.RequestDisable()) {
      out.Write("error: disable request rejected\r\n");
      return;
    }
    out.Write("ok\r\n");
    return;
  }

  if (op == "status") {
    const auto state = control_.GetState();
    if (state == app::analog::AcquisitionState::kEnabled) {
      out.Write("enabled\r\n");
      return;
    }
    out.Write("disabled\r\n");
    return;
  }

  WriteUsage(out);
}

}  // namespace app::shell::commands
