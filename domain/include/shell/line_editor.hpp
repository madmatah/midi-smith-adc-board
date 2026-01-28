#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/io/stream_requirements.hpp"

namespace shell {

template <std::size_t kBufferSize>
class LineEditor {
 public:
  LineEditor() noexcept
      : _cursor(0), _is_too_long(false), _last_newline(0), _last_was_newline(false) {
    _buffer[0] = '\0';
  }

  bool Poll(domain::io::ReadableStreamRequirements& reader,
            domain::io::WritableStreamRequirements& writer, bool& line_ready) noexcept {
    std::uint8_t byte;
    bool did_rx = false;
    line_ready = false;

    while (reader.Read(byte) == domain::io::ReadResult::kOk) {
      did_rx = true;
      if (ProcessByte(byte, writer)) {
        line_ready = true;
      }
    }

    return did_rx;
  }

  char* GetLine() noexcept {
    return _buffer;
  }

  void Reset() noexcept {
    _cursor = 0;
    _is_too_long = false;
    _buffer[0] = '\0';
  }

 private:
  bool ProcessByte(std::uint8_t byte, domain::io::WritableStreamRequirements& stream) noexcept {
    if (byte == '\r' || byte == '\n') {
      const bool is_crlf_pair = _last_was_newline && (byte != _last_newline);
      _last_newline = byte;
      _last_was_newline = true;

      if (is_crlf_pair) {
        return false;
      }

      if (_is_too_long) {
        stream.Write("\r\nerror: line too long\r\n");
        Reset();
        return false;
      }
      _buffer[_cursor] = '\0';
      stream.Write("\r\n");
      return true;
    }

    _last_was_newline = false;

    if (byte == '\b' || byte == 0x7f) {
      if (_cursor > 0) {
        _cursor--;
        stream.Write("\b \b");
      }
      return false;
    }

    // Printable characters
    if (byte >= 32 && byte <= 126) {
      if (_cursor < kBufferSize - 1) {
        _buffer[_cursor++] = static_cast<char>(byte);
        stream.Write(static_cast<char>(byte));
      } else {
        _is_too_long = true;
      }
    }

    return false;
  }

  char _buffer[kBufferSize];
  std::size_t _cursor;
  bool _is_too_long;
  std::uint8_t _last_newline;
  bool _last_was_newline;
};

}  // namespace shell
