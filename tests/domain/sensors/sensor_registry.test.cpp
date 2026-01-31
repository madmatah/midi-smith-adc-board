#if defined(UNIT_TESTS)

#include "domain/sensors/sensor_registry.hpp"

#include <catch2/catch_test_macros.hpp>

#include "domain/sensors/sensor.hpp"

TEST_CASE("The SensorRegistry class") {
  SECTION("The FindById() method") {
    SECTION("When the id exists in the registry") {
      domain::sensors::Sensor s1(1);
      domain::sensors::Sensor s2(2);
      domain::sensors::Sensor sensors[] = {s1, s2};
      domain::sensors::SensorRegistry registry(sensors, 2);

      domain::sensors::Sensor* found = registry.FindById(2);

      REQUIRE(found != nullptr);
      REQUIRE(found->id() == 2);
    }

    SECTION("When the id does not exist in the registry") {
      domain::sensors::Sensor s1(1);
      domain::sensors::Sensor sensors[] = {s1};
      domain::sensors::SensorRegistry registry(sensors, 1);

      domain::sensors::Sensor* found = registry.FindById(2);

      REQUIRE(found == nullptr);
    }

    SECTION("When the id is zero") {
      domain::sensors::Sensor s1(1);
      domain::sensors::Sensor sensors[] = {s1};
      domain::sensors::SensorRegistry registry(sensors, 1);

      domain::sensors::Sensor* found = registry.FindById(0);

      REQUIRE(found == nullptr);
    }
  }
}

#endif
