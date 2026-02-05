#include "domain/signal/filters/identity_filter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

TEST_CASE("The IdentityFilter class") {
  SECTION("The Apply() method") {
    SECTION("When called with any input value") {
      SECTION("Should return the input value unchanged") {
        domain::signal::filters::IdentityFilter filter;

        REQUIRE(filter.Apply(static_cast<std::uint16_t>(0)) == 0);
        REQUIRE(filter.Apply(static_cast<std::uint16_t>(1)) == 1);
        REQUIRE(filter.Apply(static_cast<std::uint16_t>(42)) == 42);
        REQUIRE(filter.Apply(static_cast<std::uint16_t>(65535)) == 65535);
      }
    }
  }
}
