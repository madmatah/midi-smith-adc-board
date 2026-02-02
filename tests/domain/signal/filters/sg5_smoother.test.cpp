#if defined(UNIT_TESTS)

#include "domain/signal/filters/sg5_smoother.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

TEST_CASE("The Sg5Smoother class") {
  SECTION("The Apply() method") {
    SECTION("When called with a constant signal") {
      SECTION("Should return the same constant after warmup") {
        domain::signal::filters::Sg5Smoother filter;

        for (std::uint32_t i = 0; i < 10; ++i) {
          REQUIRE(filter.Apply(1234) == 1234);
        }
      }
    }

    SECTION("When called with a ramp signal") {
      SECTION("Should return the newest sample after warmup") {
        domain::signal::filters::Sg5Smoother filter;

        REQUIRE(filter.Apply(10) == 10);
        REQUIRE(filter.Apply(20) == 20);
        REQUIRE(filter.Apply(30) == 30);
        REQUIRE(filter.Apply(40) == 40);
        REQUIRE(filter.Apply(50) == 50);
        REQUIRE(filter.Apply(60) == 60);
      }
    }
  }
}

#endif
