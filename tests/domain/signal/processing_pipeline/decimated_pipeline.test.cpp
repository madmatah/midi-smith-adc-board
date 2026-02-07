#if defined(UNIT_TESTS)

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "domain/signal/processing_pipeline.hpp"
#include "test_stubs.hpp"

TEST_CASE("The DecimatedPipeline class") {
  using Catch::Matchers::WithinAbs;
  using domain::signal::processing_pipeline::DecimatedPipeline;
  using domain::signal::processing_pipeline::test::CounterStage;

  SECTION("When configured with Factor=3 and a counter stage") {
    CounterStage::ResetCounts();
    DecimatedPipeline<3, CounterStage> pipeline;

    SECTION("Should only execute the processing logic every 3 samples") {
      pipeline.Push(10.0f);
      float out1 = pipeline.ComputeOrRaw(10.0f);
      REQUIRE_THAT(out1, WithinAbs(11.0f, 0.001f));

      pipeline.Push(20.0f);
      float out2 = pipeline.ComputeOrRaw(20.0f);
      REQUIRE_THAT(out2, WithinAbs(11.0f, 0.001f));

      pipeline.Push(30.0f);
      float out3 = pipeline.ComputeOrRaw(30.0f);
      REQUIRE_THAT(out3, WithinAbs(11.0f, 0.001f));

      pipeline.Push(40.0f);
      float out4 = pipeline.ComputeOrRaw(40.0f);
      REQUIRE_THAT(out4, WithinAbs(41.0f, 0.001f));
    }

    SECTION("Should verify CPU savings via counters") {
      REQUIRE(CounterStage::push_count == 0);
      REQUIRE(CounterStage::compute_count == 0);
      pipeline.Push(1.0f);
      pipeline.ComputeOrRaw(1.0f);
      REQUIRE(CounterStage::push_count == 1);
      REQUIRE(CounterStage::compute_count == 1);
      pipeline.Push(2.0f);
      pipeline.ComputeOrRaw(2.0f);
      REQUIRE(CounterStage::push_count == 2);
      REQUIRE(CounterStage::compute_count == 1);
      pipeline.Push(3.0f);
      pipeline.ComputeOrRaw(3.0f);
      REQUIRE(CounterStage::push_count == 3);
      REQUIRE(CounterStage::compute_count == 1);
      pipeline.Push(4.0f);
      pipeline.ComputeOrRaw(4.0f);
      REQUIRE(CounterStage::push_count == 4);
      REQUIRE(CounterStage::compute_count == 2);
    }
  }
}

#endif
