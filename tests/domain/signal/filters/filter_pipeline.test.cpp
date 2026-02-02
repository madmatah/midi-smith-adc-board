#if defined(UNIT_TESTS)

#include "domain/signal/filters/filter_pipeline.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "domain/sensors/filtering_sensor_group.hpp"
#include "domain/signal/filters/ema_filter.hpp"
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

TEST_CASE("The FilterPipeline class") {
  SECTION("The type traits") {
    SECTION("When built from Apply-only stages") {
      SECTION("Should not expose Push or ComputeOrRaw") {
        using Pipeline = domain::signal::filters::FilterPipeline<PlusOneFilter, PlusOneFilter>;
        static_assert(!domain::sensors::filtering_sensor_group_detail::HasPush<Pipeline>::value);
        static_assert(
            !domain::sensors::filtering_sensor_group_detail::HasComputeOrRaw<Pipeline>::value);
        REQUIRE(true);
      }
    }

    SECTION("When built from two-phase stages") {
      SECTION("Should expose Push and ComputeOrRaw") {
        using Pipeline =
            domain::signal::filters::FilterPipeline<domain::signal::filters::EmaFilterRatio<1, 2>,
                                                    domain::signal::filters::Sg5Smoother>;
        static_assert(domain::sensors::filtering_sensor_group_detail::HasPush<Pipeline>::value);
        static_assert(
            domain::sensors::filtering_sensor_group_detail::HasComputeOrRaw<Pipeline>::value);
        REQUIRE(true);
      }
    }
  }

  SECTION("The Apply() method") {
    SECTION("When built from Apply-only stages") {
      SECTION("Should apply all stages in order") {
        domain::signal::filters::FilterPipeline<PlusOneFilter, PlusOneFilter> pipeline;
        REQUIRE(pipeline.Apply(10) == 12);
      }
    }
  }

  SECTION("The Push() and ComputeOrRaw() methods") {
    SECTION("When built from two-phase stages") {
      SECTION("Should match manual two-stage processing") {
        using Ema = domain::signal::filters::EmaFilterRatio<1, 2>;
        using Sg = domain::signal::filters::Sg5Smoother;
        using Pipeline = domain::signal::filters::FilterPipeline<Ema, Sg>;

        Pipeline pipeline;
        Ema ema_ref;
        Sg sg_ref;

        const std::uint16_t samples[] = {10, 20, 30, 40, 50, 60, 70, 80, 90};
        for (const std::uint16_t raw : samples) {
          pipeline.Push(raw);
          const std::uint16_t pipeline_out = pipeline.ComputeOrRaw(raw);

          ema_ref.Push(raw);
          const std::uint16_t ema_out = ema_ref.ComputeOrRaw(raw);
          sg_ref.Push(ema_out);
          const std::uint16_t expected = sg_ref.ComputeOrRaw(ema_out);

          REQUIRE(pipeline_out == expected);
        }
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after processing samples") {
      SECTION("Should restore the raw fallback behavior") {
        using Pipeline =
            domain::signal::filters::FilterPipeline<domain::signal::filters::EmaFilterRatio<1, 2>,
                                                    domain::signal::filters::Sg5Smoother>;

        Pipeline pipeline;
        pipeline.Push(1234);
        (void) pipeline.ComputeOrRaw(1234);
        pipeline.Reset();

        pipeline.Push(2222);
        REQUIRE(pipeline.ComputeOrRaw(2222) == 2222);
      }
    }
  }
}

#endif
