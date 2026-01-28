#pragma once

#include <cstddef>

#include "domain/io/stream_requirements.hpp"
#include "shell/command_dispatcher.hpp"
#include "shell/command_parser.hpp"
#include "shell/line_editor.hpp"

namespace shell {

struct ShellConfig {
  const char* prompt = "shell> ";
};

template <std::size_t kLineBufferSize, std::size_t kMaxCommands, std::size_t kMaxArgs>
class ShellEngine {
 public:
  ShellEngine(domain::io::StreamRequirements& stream, const ShellConfig& config) noexcept
      : _stream(stream), _config(config) {}

  void Init() noexcept {
    ShowPrompt();
  }

  void SetPrompt(const char* prompt) noexcept {
    _config.prompt = prompt;
  }

  bool Poll() noexcept {
    bool line_ready = false;
    bool did_rx = _editor.Poll(_stream, _stream, line_ready);

    if (line_ready) {
      char* line = _editor.GetLine();
      char* argv[kMaxArgs];
      int argc = shell::CommandParser::ParseInPlace(line, kMaxArgs, argv);

      if (argc > 0) {
        _dispatcher.Dispatch(argc, argv, _stream);
      } else if (argc < 0) {
        _stream.Write("error: too many arguments\r\n");
      }

      _editor.Reset();
      ShowPrompt();
      return true;
    }

    return did_rx;
  }

  bool RegisterCommand(CommandRequirements& command) noexcept {
    return _dispatcher.Register(command);
  }

 private:
  void ShowPrompt() noexcept {
    _stream.Write(_config.prompt);
  }

  domain::io::StreamRequirements& _stream;
  ShellConfig _config;
  LineEditor<kLineBufferSize> _editor;
  CommandDispatcher<kMaxCommands> _dispatcher;
};

}  // namespace shell
