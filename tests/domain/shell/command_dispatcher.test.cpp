#include "shell/command_dispatcher.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "shell/shell_engine.hpp"

namespace {

class StreamStub : public domain::io::StreamRequirements {
 public:
  domain::io::ReadResult Read(std::uint8_t&) noexcept override {
    return domain::io::ReadResult::kNoData;
  }
  void Write(char c) noexcept override {
    _output += c;
  }
  void Write(const char* str) noexcept override {
    _output += str;
  }
  const std::string& GetOutput() const {
    return _output;
  }

 private:
  std::string _output;
};

class TestCommand : public shell::CommandRequirements {
 public:
  TestCommand(const char* name, const char* help, const char* response)
      : _name(name), _help(help), _response(response), _call_count(0) {}

  std::string_view Name() const noexcept override {
    return _name;
  }
  std::string_view Help() const noexcept override {
    return _help;
  }
  void Run(int, char**, domain::io::WritableStreamRequirements& out) noexcept override {
    _call_count++;
    out.Write(_response);
  }

  int CallCount() const {
    return _call_count;
  }

 private:
  const char* _name;
  const char* _help;
  const char* _response;
  int _call_count;
};

}  // namespace

TEST_CASE("The CommandDispatcher class", "[shell]") {
  shell::CommandDispatcher<4> dispatcher;
  StreamStub stream;
  shell::ShellConfig config{"shell> "};
  shell::ShellEngine<16, 4, 4> engine(stream, config);

  SECTION("The Register() method") {
    SECTION("Should successfully register a command") {
      TestCommand test_cmd("test", "help text", "called");
      bool ok = dispatcher.Register(test_cmd);
      REQUIRE(ok == true);
    }
  }

  SECTION("The Dispatch() method") {
    TestCommand test_cmd("test", "help text", "called");
    dispatcher.Register(test_cmd);

    SECTION("When calling an existing command") {
      char cmd[] = "test";
      char* argv[] = {cmd};
      dispatcher.Dispatch(1, argv, stream);
      REQUIRE(test_cmd.CallCount() == 1);
      REQUIRE(stream.GetOutput() == "called");
    }

    SECTION("When calling 'help'") {
      char cmd[] = "help";
      char* argv[] = {cmd};
      dispatcher.Dispatch(1, argv, stream);
      REQUIRE(stream.GetOutput().find("Available commands") != std::string::npos);
      REQUIRE(stream.GetOutput().find("test") != std::string::npos);
    }

    SECTION("When calling a non-existent command") {
      char cmd[] = "unknown";
      char* argv[] = {cmd};
      dispatcher.Dispatch(1, argv, stream);
      REQUIRE(stream.GetOutput().find("error: command not found") != std::string::npos);
    }
  }
}
