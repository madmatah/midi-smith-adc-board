#if defined(UNIT_TESTS)

#include "domain/sensors/sensor_group.hpp"

#include <catch2/catch_test_macros.hpp>

#include "domain/sensors/sensor.hpp"

TEST_CASE("The SensorGroup class") {
  SECTION("The constructor") {
    SECTION("When given an array of non-null pointers") {
      domain::sensors::Sensor s1(1);
      domain::sensors::Sensor s2(2);
      domain::sensors::Sensor* sensors[] = {&s1, &s2};

      domain::sensors::SensorGroup group(sensors, 2);

      REQUIRE(group.count() == 2);
    }
  }

  SECTION("The UpdateAt() method") {
    SECTION("When called with a valid index") {
      domain::sensors::Sensor s1(1);
      domain::sensors::Sensor s2(2);
      domain::sensors::Sensor* sensors[] = {&s1, &s2};
      domain::sensors::SensorGroup group(sensors, 2);

      group.UpdateAt(1, 1234, 99);

      REQUIRE(s2.last_raw_value() == 1234);
      REQUIRE(s2.last_filtered_value() == 1234);
      REQUIRE(s2.last_timestamp_ticks() == 99);
    }
  }
}

#endif
