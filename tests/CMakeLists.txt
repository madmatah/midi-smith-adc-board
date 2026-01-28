include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
    fakeit
    GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
    GIT_TAG 2.5.0
)
FetchContent_GetProperties(fakeit)
if(NOT fakeit_POPULATED)
    FetchContent_Populate(fakeit)
endif()

add_library(fakeit INTERFACE)
target_include_directories(fakeit INTERFACE
    ${fakeit_SOURCE_DIR}/single_header/standalone
)

add_executable(unit_tests
    test_sanity.cpp
    domain/shell/command_parser.test.cpp
    domain/shell/line_editor.test.cpp
    domain/shell/command_dispatcher.test.cpp
    domain/shell/commands/version_command.test.cpp
)
target_link_libraries(unit_tests PRIVATE
    Catch2::Catch2WithMain
    fakeit
    domain
)
target_compile_features(unit_tests PRIVATE cxx_std_17)

include(Catch)
catch_discover_tests(unit_tests)

