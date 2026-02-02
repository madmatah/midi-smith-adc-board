#if defined(UNIT_TESTS)

#include "domain/sensors/filtering_sensor_group.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "domain/signal/filters/sg5_smoother.hpp"

namespace {

class PlusOneFilter {
 public:
  void Reset() noexcept {}
  std::uint16_t Apply(std::uint16_t sample) noexcept {
    return static_cast<std::uint16_t>(sample + 1u);
  }
};

}  // namespace

TEST_CASE("The FilteringSensorGroup class") {
  SECTION("The UpdateAt() method") {
    SECTION("When called with a valid index") {
      SECTION("Should store both raw and filtered values") {
        domain::sensors::Sensor s1(1);
        domain::sensors::Sensor s2(2);
        domain::sensors::Sensor* sensors[] = {&s1, &s2};
        PlusOneFilter filters[] = {PlusOneFilter{}, PlusOneFilter{}};

        domain::sensors::FilteringSensorGroup<PlusOneFilter> group(sensors, filters, 2);

        group.UpdateAt(1, 1234, 99);

        REQUIRE(s2.last_raw_value() == 1234);
        REQUIRE(s2.last_filtered_value() == 1235);
        REQUIRE(s2.last_timestamp_ticks() == 99);
      }
    }

    SECTION("When called on a single channel repeatedly") {
      SECTION("Should apply Savitzky-Golay warmup behavior") {
        domain::sensors::Sensor s1(1);
        domain::sensors::Sensor* sensors[] = {&s1};
        domain::signal::filters::Sg5Smoother filters[] = {domain::signal::filters::Sg5Smoother{}};

        domain::sensors::FilteringSensorGroup<domain::signal::filters::Sg5Smoother> group(
            sensors, filters, 1);

        group.UpdateAt(0, 10, 1);
        REQUIRE(s1.last_filtered_value() == 10);

        group.UpdateAt(0, 20, 2);
        group.UpdateAt(0, 30, 3);
        group.UpdateAt(0, 40, 4);
        group.UpdateAt(0, 50, 5);
        REQUIRE(s1.last_filtered_value() == 50);
      }
    }
  }
}

#endif
