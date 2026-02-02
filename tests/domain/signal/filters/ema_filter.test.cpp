#if defined(UNIT_TESTS)

#include "domain/signal/filters/ema_filter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

TEST_CASE("The EmaFilterRatio class") {
  SECTION("The Apply() method") {
    SECTION("When alpha is 1") {
      SECTION("Should return the input sample") {
        domain::signal::filters::EmaFilterRatio<1, 1> filter;

        REQUIRE(filter.Apply(100) == 100);
        REQUIRE(filter.Apply(200) == 200);
        REQUIRE(filter.Apply(1234) == 1234);
        REQUIRE(filter.Apply(65535) == 65535);
      }
    }

    SECTION("When alpha is 0") {
      SECTION("Should keep the first value") {
        domain::signal::filters::EmaFilterRatio<0, 1> filter;

        REQUIRE(filter.Apply(1234) == 1234);
        REQUIRE(filter.Apply(4321) == 1234);
        REQUIRE(filter.Apply(0) == 1234);
      }
    }

    SECTION("When alpha is between 0 and 1") {
      SECTION("Should converge monotonically to a step without overshoot") {
        domain::signal::filters::EmaFilterRatio<1, 4> filter;

        REQUIRE(filter.Apply(0) == 0);

        std::uint16_t prev = filter.Apply(1000);
        REQUIRE(prev <= 1000);

        for (std::uint32_t i = 0; i < 20; ++i) {
          const std::uint16_t now = filter.Apply(1000);
          REQUIRE(now >= prev);
          REQUIRE(now <= 1000);
          prev = now;
        }
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after receiving samples") {
      SECTION("Should restore the raw fallback behavior") {
        domain::signal::filters::EmaFilterRatio<1, 2> filter;

        REQUIRE(filter.Apply(1111) == 1111);
        filter.Reset();

        REQUIRE(filter.ComputeOrRaw(2222) == 2222);
      }
    }
  }
}

#endif
