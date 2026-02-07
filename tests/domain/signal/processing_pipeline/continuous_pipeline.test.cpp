#if defined(UNIT_TESTS)

#include "domain/signal/processing_pipeline/continuous_pipeline.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "test_stubs.hpp"

TEST_CASE("The ContinuousPipeline class") {
  using Catch::Matchers::WithinAbs;
  using domain::signal::processing_pipeline::ContinuousPipeline;
  using domain::signal::processing_pipeline::test::CounterStage;
  using domain::signal::processing_pipeline::test::PlusTenStage;
  using domain::signal::processing_pipeline::test::TimesTwoStage;

  SECTION("The Process() method") {
    SECTION("When processed with a single stage") {
      ContinuousPipeline<PlusTenStage> pipeline;

      SECTION("Should apply the stage systematically on Process()") {
        REQUIRE_THAT(pipeline.Process(5.0f), WithinAbs(15.0f, 0.001f));
        REQUIRE_THAT(pipeline.Process(20.0f), WithinAbs(30.0f, 0.001f));
      }
    }

    SECTION("When processed with multiple stages") {
      ContinuousPipeline<PlusTenStage, TimesTwoStage> pipeline;

      SECTION("Should chain the stages systematically") {
        REQUIRE_THAT(pipeline.Process(5.0f), WithinAbs(30.0f, 0.001f));
        REQUIRE_THAT(pipeline.Process(6.0f), WithinAbs(32.0f, 0.001f));
      }
    }
  }

  SECTION("The Reset() method") {
    CounterStage::ResetCounts();
    ContinuousPipeline<CounterStage> pipeline;

    SECTION("Should reset the stages systematically") {
      pipeline.Process(1.0f);
      REQUIRE(CounterStage::reset_count == 0);
      pipeline.Reset();
      REQUIRE(CounterStage::reset_count == 1);
    }
  }
}

#endif
