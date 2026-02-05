#if defined(UNIT_TESTS)

#include "app/analog/adc_rank_mapped_frame_decoder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "app/config/sensors.hpp"
#include "domain/sensors/sensor.hpp"
#include "domain/sensors/sensor_group.hpp"

TEST_CASE("The AdcRankMappedFrameDecoder class") {
  SECTION("The ApplySequence() method") {
    domain::sensors::Sensor s[22] = {
        domain::sensors::Sensor{1},  domain::sensors::Sensor{2},  domain::sensors::Sensor{3},
        domain::sensors::Sensor{4},  domain::sensors::Sensor{5},  domain::sensors::Sensor{6},
        domain::sensors::Sensor{7},  domain::sensors::Sensor{8},  domain::sensors::Sensor{9},
        domain::sensors::Sensor{10}, domain::sensors::Sensor{11}, domain::sensors::Sensor{12},
        domain::sensors::Sensor{13}, domain::sensors::Sensor{14}, domain::sensors::Sensor{15},
        domain::sensors::Sensor{16}, domain::sensors::Sensor{17}, domain::sensors::Sensor{18},
        domain::sensors::Sensor{19}, domain::sensors::Sensor{20}, domain::sensors::Sensor{21},
        domain::sensors::Sensor{22},
    };
    domain::sensors::Sensor* ptrs[22] = {
        &s[0],  &s[1],  &s[2],  &s[3],  &s[4],  &s[5],  &s[6],  &s[7],  &s[8],  &s[9],  &s[10],
        &s[11], &s[12], &s[13], &s[14], &s[15], &s[16], &s[17], &s[18], &s[19], &s[20], &s[21],
    };
    domain::sensors::SensorGroup group(ptrs, 22);

    SECTION("When called with one ADC1 sequence") {
      const std::uint16_t values[7] = {101, 103, 105, 107, 109, 111, 112};

      app::analog::AdcRankMappedFrameDecoder decoder;
      decoder.ApplySequence(values, 7, app::config_sensors::kAdc1SensorIdByRank, group, 123u);

      REQUIRE(s[0].last_raw_value() == 101);
      REQUIRE(s[2].last_raw_value() == 103);
      REQUIRE(s[4].last_raw_value() == 105);
      REQUIRE(s[6].last_raw_value() == 107);
      REQUIRE(s[8].last_raw_value() == 109);
      REQUIRE(s[10].last_raw_value() == 111);
      REQUIRE(s[11].last_raw_value() == 112);
      REQUIRE(s[11].last_timestamp_ticks() == 123u);
    }

    SECTION("When called with one ADC3 sequence") {
      const std::uint16_t values[8] = {213, 214, 217, 218, 219, 220, 221, 222};

      app::analog::AdcRankMappedFrameDecoder decoder;
      decoder.ApplySequence(values, 8, app::config_sensors::kAdc3SensorIdByRank, group, 987u);

      REQUIRE(s[12].last_raw_value() == 213);
      REQUIRE(s[21].last_raw_value() == 222);
      REQUIRE(s[21].last_timestamp_ticks() == 987u);
    }
  }
}

#endif
