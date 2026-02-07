#pragma once

#include <concepts>
#include <type_traits>

namespace domain::signal {

template <typename T>
concept ResettableSignalProcessor = requires(T t) {
  { t.Reset() } -> std::same_as<void>;
};

template <typename T>
concept SignalProcessor = ResettableSignalProcessor<T> && requires(T t, float input) {
  { t.Process(input) } -> std::same_as<float>;
};

template <typename T>
concept DecimationCompatibleSignalProcessor =
    ResettableSignalProcessor<T> && requires(T t, const T ct, float input) {
      { t.Push(input) } -> std::same_as<void>;
      { ct.ComputeOrRaw(input) } -> std::same_as<float>;
    };

template <typename T>
struct is_signal_processor : std::bool_constant<SignalProcessor<T>> {};

template <typename T>
struct is_decimation_compatible : std::bool_constant<DecimationCompatibleSignalProcessor<T>> {};

}  // namespace domain::signal
