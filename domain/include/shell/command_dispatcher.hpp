#pragma once

#include <cstddef>
#include <cstring>
#include <string_view>

#include "domain/io/stream_requirements.hpp"
#include "shell/command_requirements.hpp"

namespace shell {

template <std::size_t kMaxCommands>
class CommandDispatcher {
 public:
  CommandDispatcher() noexcept : _count(0) {}

  bool Register(CommandRequirements& command) noexcept {
    if (_count >= kMaxCommands) {
      return false;
    }
    _commands[_count++] = &command;
    return true;
  }

  void Dispatch(int argc, char** argv, domain::io::WritableStreamRequirements& out) const noexcept {
    if (argc <= 0) {
      return;
    }

    std::string_view cmd_name(argv[0]);

    if (cmd_name == "help") {
      ShowHelp(out);
      return;
    }

    for (std::size_t i = 0; i < _count; ++i) {
      if (cmd_name == _commands[i]->Name()) {
        _commands[i]->Run(argc, argv, out);
        return;
      }
    }

    out.Write("error: command not found: ");
    out.Write(argv[0]);
    out.Write("\r\n");
  }

  void ShowHelp(domain::io::WritableStreamRequirements& out) const noexcept {
    out.Write("Available commands:\r\n");
    out.Write("  help                Show this help message\r\n");
    for (std::size_t i = 0; i < _count; ++i) {
      out.Write("  ");
      std::string_view name = _commands[i]->Name();
      out.Write(name);

      // Pad with spaces for alignment
      const std::size_t kNameColumnWidth = 20;
      for (std::size_t j = name.length(); j < kNameColumnWidth; ++j) {
        out.Write(' ');
      }

      out.Write(_commands[i]->Help());
      out.Write("\r\n");
    }
  }

 private:
  CommandRequirements* _commands[kMaxCommands];
  std::size_t _count;
};

}  // namespace shell
