#pragma once

#include <cstdint>
#include <initializer_list>
#include <tuple>
#include <type_traits>
#include <utility>

namespace domain::signal::filters {
namespace filter_pipeline_detail {

template <typename T, typename = void>
struct HasApply : std::false_type {};

template <typename T>
struct HasApply<T, std::void_t<decltype(std::declval<T&>().Apply(std::declval<std::uint16_t>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct HasPush : std::false_type {};

template <typename T>
struct HasPush<T, std::void_t<decltype(std::declval<T&>().Push(std::declval<std::uint16_t>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct HasComputeOrRaw : std::false_type {};

template <typename T>
struct HasComputeOrRaw<
    T, std::void_t<decltype(std::declval<const T&>().ComputeOrRaw(std::declval<std::uint16_t>()))>>
    : std::true_type {};

template <typename T, typename = void>
struct HasReset : std::false_type {};

template <typename T>
struct HasReset<T, std::void_t<decltype(std::declval<T&>().Reset())>> : std::true_type {};

template <typename T>
struct IsTwoPhase : std::integral_constant<bool, HasPush<T>::value && HasComputeOrRaw<T>::value> {};

template <typename T>
inline constexpr bool IsTwoPhaseV = IsTwoPhase<T>::value;

template <typename FilterT>
inline void ResetIfPresent(FilterT& filter) noexcept {
  if constexpr (HasReset<FilterT>::value) {
    filter.Reset();
  }
}

template <typename FilterT>
inline std::uint16_t ApplyOrPushCompute(FilterT& filter, std::uint16_t input) noexcept {
  if constexpr (IsTwoPhaseV<FilterT>) {
    filter.Push(input);
    return filter.ComputeOrRaw(input);
  } else if constexpr (HasApply<FilterT>::value) {
    return filter.Apply(input);
  } else {
    static_assert(std::is_same_v<FilterT, void>,
                  "Filter must provide Apply() or Push()+ComputeOrRaw()");
    return input;
  }
}

template <typename TupleT, std::size_t... kIs>
inline void ResetAll(TupleT& filters, std::index_sequence<kIs...>) noexcept {
  (ResetIfPresent(std::get<kIs>(filters)), ...);
}

template <typename TupleT, std::size_t... kIs>
inline std::uint16_t ApplyAll(TupleT& filters, std::uint16_t input,
                              std::index_sequence<kIs...>) noexcept {
  std::uint16_t x = input;
  (void) std::initializer_list<int>{((x = ApplyOrPushCompute(std::get<kIs>(filters), x)), 0)...};
  return x;
}

template <typename TupleT, std::size_t kLastIndex>
inline std::uint16_t PushThroughPreStages(TupleT& filters, std::uint16_t input) noexcept {
  if constexpr (kLastIndex == 0u) {
    return input;
  } else {
    return ApplyAll(filters, input, std::make_index_sequence<kLastIndex>{});
  }
}

}  // namespace filter_pipeline_detail

template <typename... FilterTs>
class FilterPipelineBase {
 protected:
  static_assert(sizeof...(FilterTs) >= 2u, "FilterPipeline must have at least 2 stages");

  using FiltersTuple = std::tuple<FilterTs...>;
  static constexpr std::size_t kStageCount = sizeof...(FilterTs);
  static constexpr std::size_t kLastIndex = kStageCount - 1u;

  FilterPipelineBase() = default;

  explicit FilterPipelineBase(FilterTs... filters) noexcept : filters_(std::move(filters)...) {}

  void ResetAllStages() noexcept {
    filter_pipeline_detail::ResetAll(filters_, std::make_index_sequence<kStageCount>{});
  }

  FiltersTuple filters_{};
};

template <typename... FilterTs>
class FilterPipeline : public FilterPipelineBase<FilterTs...> {
  static constexpr bool kHasTwoPhaseStage = (filter_pipeline_detail::IsTwoPhaseV<FilterTs> || ...);

 public:
  using FilterPipelineBase<FilterTs...>::FilterPipelineBase;

  void Reset() noexcept {
    this->ResetAllStages();
    has_last_input_ = false;
    last_input_ = 0;
    has_last_output_ = false;
    last_output_ = 0;
  }

  std::uint16_t Apply(std::uint16_t sample) noexcept {
    if constexpr (kHasTwoPhaseStage) {
      Push(sample);
      return ComputeOrRaw(sample);
    } else {
      return filter_pipeline_detail::ApplyAll(
          this->filters_, sample,
          std::make_index_sequence<FilterPipelineBase<FilterTs...>::kStageCount>{});
    }
  }

  template <bool kEnable = kHasTwoPhaseStage, typename = std::enable_if_t<kEnable>>
  void Push(std::uint16_t sample) noexcept {
    std::uint16_t x = filter_pipeline_detail::PushThroughPreStages<
        typename FilterPipelineBase<FilterTs...>::FiltersTuple,
        FilterPipelineBase<FilterTs...>::kLastIndex>(this->filters_, sample);

    using LastT = std::tuple_element_t<FilterPipelineBase<FilterTs...>::kLastIndex,
                                       typename FilterPipelineBase<FilterTs...>::FiltersTuple>;
    LastT& last = std::get<FilterPipelineBase<FilterTs...>::kLastIndex>(this->filters_);

    if constexpr (filter_pipeline_detail::HasPush<LastT>::value) {
      last.Push(x);
    } else if constexpr (filter_pipeline_detail::HasApply<LastT>::value) {
      last_output_ = last.Apply(x);
      has_last_output_ = true;
    } else {
      static_assert(std::is_same_v<LastT, void>, "Last stage must provide Apply() or Push()");
    }

    last_input_ = x;
    has_last_input_ = true;
  }

  template <bool kEnable = kHasTwoPhaseStage, typename = std::enable_if_t<kEnable>>
  std::uint16_t ComputeOrRaw(std::uint16_t raw_fallback) const noexcept {
    if (!has_last_input_) {
      return raw_fallback;
    }

    using LastT = std::tuple_element_t<FilterPipelineBase<FilterTs...>::kLastIndex,
                                       typename FilterPipelineBase<FilterTs...>::FiltersTuple>;
    const LastT& last = std::get<FilterPipelineBase<FilterTs...>::kLastIndex>(this->filters_);

    if constexpr (filter_pipeline_detail::HasComputeOrRaw<LastT>::value) {
      return last.ComputeOrRaw(last_input_);
    } else {
      if (!has_last_output_) {
        return last_input_;
      }
      return last_output_;
    }
  }

 private:
  bool has_last_input_ = false;
  std::uint16_t last_input_ = 0;
  bool has_last_output_ = false;
  std::uint16_t last_output_ = 0;
};

}  // namespace domain::signal::filters
