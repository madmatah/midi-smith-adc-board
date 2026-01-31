#if defined(UNIT_TESTS)

#include "app/analog/adc12_frame_decoder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "domain/sensors/sensor.hpp"
#include "domain/sensors/sensor_group.hpp"

TEST_CASE("The Adc12FrameDecoder class") {
  SECTION("The ApplySequence() method") {
    SECTION("When called with one packed sequence") {
      domain::sensors::Sensor s[14] = {
          domain::sensors::Sensor{1},  domain::sensors::Sensor{2},  domain::sensors::Sensor{3},
          domain::sensors::Sensor{4},  domain::sensors::Sensor{5},  domain::sensors::Sensor{6},
          domain::sensors::Sensor{7},  domain::sensors::Sensor{8},  domain::sensors::Sensor{9},
          domain::sensors::Sensor{10}, domain::sensors::Sensor{12}, domain::sensors::Sensor{13},
          domain::sensors::Sensor{11}, domain::sensors::Sensor{14},
      };
      domain::sensors::Sensor* ptrs[14] = {&s[0], &s[1], &s[2], &s[3],  &s[4],  &s[5],  &s[6],
                                           &s[7], &s[8], &s[9], &s[10], &s[11], &s[12], &s[13]};
      domain::sensors::SensorGroup group(ptrs, 14);

      const std::uint16_t a1[7] = {101, 103, 105, 107, 109, 112, 111};
      const std::uint16_t a2[7] = {102, 104, 106, 108, 110, 113, 114};
      std::uint32_t words[7] = {};
      for (std::size_t i = 0; i < 7; ++i) {
        words[i] = (static_cast<std::uint32_t>(a2[i]) << 16) | static_cast<std::uint32_t>(a1[i]);
      }

      app::analog::Adc12FrameDecoder decoder;
      decoder.ApplySequence(words, 7, group, 123u);

      REQUIRE(s[0].last_raw_value() == 101);
      REQUIRE(s[1].last_raw_value() == 102);
      REQUIRE(s[2].last_raw_value() == 103);
      REQUIRE(s[3].last_raw_value() == 104);
      REQUIRE(s[10].last_raw_value() == 112);
      REQUIRE(s[11].last_raw_value() == 113);
      REQUIRE(s[12].last_raw_value() == 111);
      REQUIRE(s[13].last_raw_value() == 114);
    }
  }
}

#endif
