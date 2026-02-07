#pragma once

#include <cstdint>
#include <type_traits>

#include "domain/signal/filters/ema_filter.hpp"
#include "domain/signal/filters/identity_filter.hpp"
#include "domain/signal/filters/sg5_smoother.hpp"
#include "domain/signal/processing_pipeline.hpp"

namespace app::config {

constexpr bool SIGNAL_FILTERING_ENABLED = true;

constexpr std::int32_t SIGNAL_EMA_ALPHA_NUMERATOR = 1;
constexpr std::int32_t SIGNAL_EMA_ALPHA_DENOMINATOR = 8;

// Decimation factor is applied on segments of the pipeline to reduce processing frequency.
// Set to 1 to disable decimation.
constexpr std::uint8_t SIGNAL_DECIMATION_FACTOR = 1;

namespace signal_filtering_detail {

using FilteringEnabledPipeline =
    domain::signal::processing_pipeline::ContinuousPipeline<domain::signal::filters::EmaFilterRatio<
        SIGNAL_EMA_ALPHA_NUMERATOR, SIGNAL_EMA_ALPHA_DENOMINATOR>>;

using FilteringDisabledPipeline = domain::signal::processing_pipeline::ContinuousPipeline<
    domain::signal::filters::IdentityFilter>;

using FilteringPipeline =
    std::conditional_t<SIGNAL_FILTERING_ENABLED, signal_filtering_detail::FilteringEnabledPipeline,
                       signal_filtering_detail::FilteringDisabledPipeline>;

}  // namespace signal_filtering_detail

using AnalogSensorProcessor = domain::signal::processing_pipeline::SignalProcessingPipeline<
    signal_filtering_detail::FilteringPipeline>;

}  // namespace app::config
