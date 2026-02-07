#pragma once

#include <concepts>
#include <utility>

namespace domain::signal::processing_pipeline::detail {

template <typename T>
concept Resettable = requires(T t) {
  { t.Reset() } -> std::same_as<void>;
};

template <typename StageT>
inline void ResetIfPresent(StageT& stage) noexcept {
  if constexpr (Resettable<StageT>) {
    stage.Reset();
  }
}

template <typename TupleT, std::size_t... kIs>
inline void ResetAll(TupleT& stages, std::index_sequence<kIs...>) noexcept {
  (ResetIfPresent(std::get<kIs>(stages)), ...);
}

template <typename TupleT, std::size_t... kIs>
inline float ProcessAll(TupleT& stages, float input, std::index_sequence<kIs...>) noexcept {
  float x = input;
  ((x = std::get<kIs>(stages).Process(x)), ...);
  return x;
}

template <typename TupleT, std::size_t... kIs>
inline void PushAll(TupleT& stages, float input, std::index_sequence<kIs...>) noexcept {
  (std::get<kIs>(stages).Push(input), ...);
}

template <typename TupleT, std::size_t... kIs>
inline float ComputeAll(const TupleT& stages, float input, std::index_sequence<kIs...>) noexcept {
  float x = input;
  ((x = std::get<kIs>(stages).ComputeOrRaw(x)), ...);
  return x;
}

}  // namespace domain::signal::processing_pipeline::detail
