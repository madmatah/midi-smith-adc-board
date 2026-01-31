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
    domain/shell/command_parser.test.cpp
    domain/shell/line_editor.test.cpp
    domain/shell/command_dispatcher.test.cpp
    domain/shell/commands/version_command.test.cpp
    domain/sensors/sensor_group.test.cpp
    domain/sensors/sensor_registry.test.cpp
    app/analog/adc12_frame_decoder.test.cpp
    app/analog/adc3_frame_decoder.test.cpp
    app/analog/acquisition_sequencer.test.cpp
)
target_link_libraries(unit_tests PRIVATE
    Catch2::Catch2WithMain
    fakeit
    domain
)
target_include_directories(unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/app/include
    ${CMAKE_SOURCE_DIR}/bsp/include
)
target_compile_definitions(unit_tests PRIVATE UNIT_TESTS=1)
target_compile_features(unit_tests PRIVATE cxx_std_17)

include(Catch)
catch_discover_tests(unit_tests)

