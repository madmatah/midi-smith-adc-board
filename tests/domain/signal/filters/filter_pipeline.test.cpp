#if defined(UNIT_TESTS)

#include "domain/signal/filters/filter_pipeline.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

#include "domain/sensors/filtering_sensor_group.hpp"
#include "domain/signal/filters/ema_filter.hpp"
#include "domain/signal/filters/sg5_smoother.hpp"

namespace {

class PlusOneFilter {
 public:
  void Reset() noexcept {}

  float Apply(float sample) noexcept {
    return sample + 1.0f;
  }
};

}  // namespace

TEST_CASE("The FilterPipeline class") {
  using Catch::Matchers::WithinAbs;

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
        REQUIRE_THAT(pipeline.Apply(10.0f), WithinAbs(12.0f, 0.001f));
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

        const float samples[] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f, 60.0f, 70.0f, 80.0f, 90.0f};
        for (const float raw : samples) {
          pipeline.Push(raw);
          const float pipeline_out = pipeline.ComputeOrRaw(raw);

          ema_ref.Push(raw);
          const float ema_out = ema_ref.ComputeOrRaw(raw);
          sg_ref.Push(ema_out);
          const float expected = sg_ref.ComputeOrRaw(ema_out);

          REQUIRE_THAT(pipeline_out, WithinAbs(expected, 0.001f));
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
        pipeline.Push(1234.0f);
        (void) pipeline.ComputeOrRaw(1234.0f);
        pipeline.Reset();

        pipeline.Push(2222.0f);
        REQUIRE_THAT(pipeline.ComputeOrRaw(2222.0f), WithinAbs(2222.0f, 0.001f));
      }
    }
  }
}

#endif
