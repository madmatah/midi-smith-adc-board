#pragma once

#include <cstddef>
#include <cstring>

#include "domain/io/stream_requirements.hpp"

namespace shell {

struct ShellCommand {
  const char* name;
  const char* help;
  void (*handler)(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept;
};

template <std::size_t kMaxCommands>
class CommandDispatcher {
 public:
  CommandDispatcher() noexcept : _count(0) {}

  bool Register(const ShellCommand& command) noexcept {
    if (_count >= kMaxCommands) {
      return false;
    }
    _commands[_count++] = command;
    return true;
  }

  void Dispatch(int argc, char** argv, domain::io::WritableStreamRequirements& out) const noexcept {
    if (argc <= 0) {
      return;
    }

    if (std::strcmp(argv[0], "help") == 0) {
      ShowHelp(out);
      return;
    }

    for (std::size_t i = 0; i < _count; ++i) {
      if (std::strcmp(argv[0], _commands[i].name) == 0) {
        _commands[i].handler(argc, argv, out);
        return;
      }
    }

    out.Write("error: command not found: ");
    out.Write(argv[0]);
    out.Write("\r\n");
  }

  void ShowHelp(domain::io::WritableStreamRequirements& out) const noexcept {
    out.Write("Available commands:\r\n");
    out.Write("  help\t\tShow this help message\r\n");
    for (std::size_t i = 0; i < _count; ++i) {
      out.Write("  ");
      out.Write(_commands[i].name);
      out.Write("\t\t");
      out.Write(_commands[i].help);
      out.Write("\r\n");
    }
  }

 private:
  ShellCommand _commands[kMaxCommands];
  std::size_t _count;
};

}  // namespace shell
