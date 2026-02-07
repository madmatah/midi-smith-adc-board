#if defined(UNIT_TESTS)

#include "domain/signal/filters/sg5_smoother.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

TEST_CASE("The Sg5Smoother class") {
  using Catch::Matchers::WithinAbs;

  SECTION("The Process() method") {
    SECTION("When called with a constant signal") {
      SECTION("Should return the same constant after warmup") {
        domain::signal::filters::Sg5Smoother filter;

        for (std::uint32_t i = 0; i < 10; ++i) {
          REQUIRE_THAT(filter.Process(1234.0f), WithinAbs(1234.0f, 0.001f));
        }
      }
    }

    SECTION("When called with a ramp signal") {
      SECTION("Should return the newest sample after warmup") {
        domain::signal::filters::Sg5Smoother filter;

        REQUIRE_THAT(filter.Process(10.0f), WithinAbs(10.0f, 0.001f));
        REQUIRE_THAT(filter.Process(20.0f), WithinAbs(20.0f, 0.001f));
        REQUIRE_THAT(filter.Process(30.0f), WithinAbs(30.0f, 0.001f));
        REQUIRE_THAT(filter.Process(40.0f), WithinAbs(40.0f, 0.001f));
        REQUIRE_THAT(filter.Process(50.0f), WithinAbs(50.0f, 0.001f));
        REQUIRE_THAT(filter.Process(60.0f), WithinAbs(60.0f, 0.001f));
      }
    }
  }
}

#endif
