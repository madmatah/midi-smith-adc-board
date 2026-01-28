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

static int test_call_count = 0;
static void TestHandler(int argc, char** argv,
                        domain::io::WritableStreamRequirements& out) noexcept {
  test_call_count++;
  out.Write("called");
}

}  // namespace

TEST_CASE("The CommandDispatcher class", "[shell]") {
  shell::CommandDispatcher<4> dispatcher;
  StreamStub stream;
  shell::ShellConfig config{"shell> "};
  shell::ShellEngine<16, 4, 4> engine(stream, config);

  SECTION("The Register() method") {
    SECTION("Should successfully register a command") {
      bool ok = dispatcher.Register({"test", "help text", TestHandler});
      REQUIRE(ok == true);
    }
  }

  SECTION("The Dispatch() method") {
    dispatcher.Register({"test", "help text", TestHandler});
    test_call_count = 0;

    SECTION("When calling an existing command") {
      char cmd[] = "test";
      char* argv[] = {cmd};
      dispatcher.Dispatch(1, argv, stream);
      REQUIRE(test_call_count == 1);
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
