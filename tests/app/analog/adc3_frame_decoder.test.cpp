#if defined(UNIT_TESTS)

#include "app/analog/adc3_frame_decoder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "domain/sensors/sensor.hpp"
#include "domain/sensors/sensor_group.hpp"

TEST_CASE("The Adc3FrameDecoder class") {
  SECTION("The ApplySequence() method") {
    SECTION("When called with one sequence") {
      domain::sensors::Sensor s[8] = {
          domain::sensors::Sensor{15}, domain::sensors::Sensor{16}, domain::sensors::Sensor{17},
          domain::sensors::Sensor{18}, domain::sensors::Sensor{19}, domain::sensors::Sensor{20},
          domain::sensors::Sensor{21}, domain::sensors::Sensor{22},
      };
      domain::sensors::Sensor* ptrs[8] = {&s[0], &s[1], &s[2], &s[3], &s[4], &s[5], &s[6], &s[7]};
      domain::sensors::SensorGroup group(ptrs, 8);

      const std::uint16_t values[8] = {215, 216, 217, 218, 219, 220, 221, 222};

      app::analog::Adc3FrameDecoder decoder;
      decoder.ApplySequence(values, 8, group, 987u);

      REQUIRE(s[0].last_raw_value() == 215);
      REQUIRE(s[7].last_raw_value() == 222);
      REQUIRE(s[7].last_timestamp_ticks() == 987u);
    }
  }
}

#endif
