#pragma once

#include "domain/io/stream_requirements.hpp"
#include "shell/shell_engine.hpp"

namespace app::Tasks {

class ShellTask {
 public:
  explicit ShellTask(domain::io::StreamRequirements& stream,
                     const shell::ShellConfig& config) noexcept;

  static void entry(void* ctx) noexcept;
  void run() noexcept;
  bool start() noexcept;

  bool RegisterCommand(shell::CommandRequirements& command) noexcept {
    return _engine.RegisterCommand(command);
  }

 private:
  static constexpr std::size_t kLineBufferSize = 128;
  static constexpr std::size_t kMaxCommands = 16;
  static constexpr std::size_t kMaxArgs = 8;

  shell::ShellEngine<kLineBufferSize, kMaxCommands, kMaxArgs> _engine;
};

}  // namespace app::Tasks
