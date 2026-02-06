#include "domain/signal/filters/identity_filter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

TEST_CASE("The IdentityFilter class") {
  using Catch::Matchers::WithinRel;

  SECTION("The Apply() method") {
    SECTION("When called with any input value") {
      SECTION("Should return the input value unchanged") {
        domain::signal::filters::IdentityFilter filter;

        REQUIRE_THAT(filter.Apply(0.0f), WithinRel(0.0f));
        REQUIRE_THAT(filter.Apply(1.0f), WithinRel(1.0f));
        REQUIRE_THAT(filter.Apply(42.0f), WithinRel(42.0f));
        REQUIRE_THAT(filter.Apply(65535.0f), WithinRel(65535.0f));
      }
    }
  }
}
