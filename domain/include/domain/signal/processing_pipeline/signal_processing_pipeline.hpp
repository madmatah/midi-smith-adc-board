#pragma once

#include "domain/signal/processing_pipeline/continuous_pipeline.hpp"
#include "domain/signal/signal_processor_concepts.hpp"

namespace domain::signal::processing_pipeline {

template <SignalProcessor... StageTs>
using SignalProcessingPipeline = ContinuousPipeline<StageTs...>;

}  // namespace domain::signal::processing_pipeline
