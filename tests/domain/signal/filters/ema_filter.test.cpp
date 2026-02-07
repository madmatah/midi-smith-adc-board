#if defined(UNIT_TESTS)

#include "domain/signal/filters/ema_filter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

TEST_CASE("The EmaFilterRatio class") {
  SECTION("The Process() method") {
    SECTION("When alpha is 1") {
      SECTION("Should return the input sample") {
        domain::signal::filters::EmaFilterRatio<1, 1> filter;
        using Catch::Matchers::WithinRel;

        REQUIRE_THAT(filter.Process(100.0f), WithinRel(100.0f));
        REQUIRE_THAT(filter.Process(200.0f), WithinRel(200.0f));
        REQUIRE_THAT(filter.Process(1234.0f), WithinRel(1234.0f));
        REQUIRE_THAT(filter.Process(65535.0f), WithinRel(65535.0f));
      }
    }

    SECTION("When alpha is 0") {
      SECTION("Should keep the first value") {
        domain::signal::filters::EmaFilterRatio<0, 1> filter;
        using Catch::Matchers::WithinRel;

        REQUIRE_THAT(filter.Process(1234.0f), WithinRel(1234.0f));
        REQUIRE_THAT(filter.Process(4321.0f), WithinRel(1234.0f));
        REQUIRE_THAT(filter.Process(0.0f), WithinRel(1234.0f));
      }
    }

    SECTION("When alpha is between 0 and 1") {
      SECTION("Should converge monotonically to a step without overshoot") {
        domain::signal::filters::EmaFilterRatio<1, 4> filter;

        using Catch::Matchers::WithinAbs;
        REQUIRE_THAT(filter.Process(0.0f), WithinAbs(0.0f, 0.0001f));

        float prev = filter.Process(1000.0f);
        REQUIRE(prev <= 1000.0f);

        for (std::uint32_t i = 0; i < 20; ++i) {
          const float now = filter.Process(1000.0f);
          REQUIRE(now >= prev);
          REQUIRE(now <= 1000.0f);
          prev = now;
        }
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after receiving samples") {
      SECTION("Should restore the raw fallback behavior") {
        domain::signal::filters::EmaFilterRatio<1, 2> filter;

        REQUIRE_THAT(filter.Process(1111.0f), Catch::Matchers::WithinRel(1111.0f));
        filter.Reset();

        REQUIRE_THAT(filter.ComputeOrRaw(2222.0f), Catch::Matchers::WithinRel(2222.0f));
      }
    }
  }
}

#endif
