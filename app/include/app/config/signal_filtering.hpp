#pragma once

#include <cstdint>
#include <type_traits>

#include "domain/signal/filters/ema_filter.hpp"
#include "domain/signal/filters/filter_pipeline.hpp"
#include "domain/signal/filters/identity_filter.hpp"
#include "domain/signal/filters/sg5_smoother.hpp"

namespace app::config {

constexpr bool SIGNAL_FILTERING_ENABLED = true;

constexpr std::int32_t SIGNAL_EMA_ALPHA_NUMERATOR = 1;
constexpr std::int32_t SIGNAL_EMA_ALPHA_DENOMINATOR = 16;

namespace signal_filtering_detail {

using EnabledAnalogSensorFilter = domain::signal::filters::FilterPipeline<
    domain::signal::filters::EmaFilterRatio<SIGNAL_EMA_ALPHA_NUMERATOR,
                                            SIGNAL_EMA_ALPHA_DENOMINATOR>,
    domain::signal::filters::Sg5Smoother>;

using DisabledAnalogSensorFilter = domain::signal::filters::IdentityFilter;

}  // namespace signal_filtering_detail

using AnalogSensorFilter =
    std::conditional_t<SIGNAL_FILTERING_ENABLED, signal_filtering_detail::EnabledAnalogSensorFilter,
                       signal_filtering_detail::DisabledAnalogSensorFilter>;


// Decimation factor is applied on the filtered signal to reduce the sampling rate
// This is useful for reducing the CPU load of the filtering task. Set to 1 to disable decimation.
constexpr std::uint8_t SIGNAL_DECIMATION_FACTOR = 1;


}  // namespace app::config
