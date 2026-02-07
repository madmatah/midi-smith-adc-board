#if defined(UNIT_TESTS)

#include "domain/signal/processors/tia_current_converter.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

#include "domain/signal/signal_processor_concepts.hpp"

namespace {
constexpr std::int32_t kTestVrefMilliVolts = 2048;
constexpr std::int32_t kTestAdcBits = 16;
constexpr std::int32_t kTestRfOhms = 1800;

using TestConverter =
    domain::signal::processors::TiaCurrentConverter<kTestVrefMilliVolts, kTestAdcBits, kTestRfOhms>;

constexpr float kAdcMaxCounts = 65535.0f;
constexpr float kExpectedSaturationCurrentMa =
    static_cast<float>(kTestVrefMilliVolts) / static_cast<float>(kTestRfOhms);
constexpr float kScaleFactorPrecisionTolerance = 1e-4f;
constexpr float kCurrentToleranceMa = 1e-5f;
}  // namespace

TEST_CASE("The TiaCurrentConverter class") {
  SECTION("Polarity and rest point") {
    SECTION("When input is at maximum ADC counts (rest point)") {
      SECTION("Should output exactly zero current in mA") {
        TestConverter converter;

        float const output_ma = converter.Process(kAdcMaxCounts);

        using Catch::Matchers::WithinAbs;
        REQUIRE_THAT(output_ma, WithinAbs(0.0f, kCurrentToleranceMa));
      }
    }

    SECTION("When input is at zero ADC counts (saturation)") {
      SECTION("Should output theoretical saturation current Vref_mV / Rf_Ohms") {
        TestConverter converter;

        float const output_ma = converter.Process(0.0f);

        using Catch::Matchers::WithinAbs;
        REQUIRE_THAT(output_ma, WithinAbs(kExpectedSaturationCurrentMa, kCurrentToleranceMa));
      }
    }
  }

  SECTION("Mathematical precision") {
    SECTION("Linearity at mid-scale") {
      SECTION("When input is 50% of ADC range") {
        SECTION("Should output 50% of max current") {
          TestConverter converter;
          float const half_scale_adc = kAdcMaxCounts * 0.5f;
          float const expected_half_current_ma = kExpectedSaturationCurrentMa * 0.5f;

          float const output_ma = converter.Process(half_scale_adc);

          using Catch::Matchers::WithinAbs;
          REQUIRE_THAT(output_ma, WithinAbs(expected_half_current_ma, kCurrentToleranceMa));
        }
      }
    }

    SECTION("Scale factor precision") {
      SECTION("When input is one count below maximum") {
        SECTION("Should have gain within 1e-4 of theoretical per-count current") {
          TestConverter converter;
          float const one_count_current = converter.Process(kAdcMaxCounts - 1.0f);
          float const expected_per_count_ma = static_cast<float>(kTestVrefMilliVolts) /
                                              (kAdcMaxCounts * static_cast<float>(kTestRfOhms));

          using Catch::Matchers::WithinAbs;
          REQUIRE_THAT(one_count_current,
                       WithinAbs(expected_per_count_ma, kScaleFactorPrecisionTolerance));
        }
      }
    }
  }

  SECTION("Boundary robustness") {
    SECTION("At ADC value 0") {
      SECTION("Should not underflow and should return positive saturation current") {
        TestConverter converter;

        float const output_ma = converter.Process(0.0f);

        REQUIRE(output_ma > 0.0f);
        using Catch::Matchers::WithinAbs;
        REQUIRE_THAT(output_ma, WithinAbs(kExpectedSaturationCurrentMa, kCurrentToleranceMa));
      }
    }

    SECTION("At ADC value MaxCounts") {
      SECTION("Should not overflow and should return zero current") {
        TestConverter converter;

        float const output_ma = converter.Process(kAdcMaxCounts);

        REQUIRE(output_ma >= 0.0f);
        using Catch::Matchers::WithinAbs;
        REQUIRE_THAT(output_ma, WithinAbs(0.0f, kCurrentToleranceMa));
      }
    }
  }

  SECTION("Template parameter adaptation") {
    SECTION("When using 12-bit ADC resolution") {
      SECTION("Should compute correct max value and same saturation current formula") {
        using Converter12 =
            domain::signal::processors::TiaCurrentConverter<kTestVrefMilliVolts, 12, kTestRfOhms>;
        Converter12 converter_12;
        float const adc_max_12 = static_cast<float>((1ULL << 12) - 1);
        float const expected_saturation_12 =
            static_cast<float>(kTestVrefMilliVolts) / static_cast<float>(kTestRfOhms);

        float const zero_at_max = converter_12.Process(adc_max_12);
        float const saturation_at_zero = converter_12.Process(0.0f);

        using Catch::Matchers::WithinAbs;
        REQUIRE_THAT(zero_at_max, WithinAbs(0.0f, kCurrentToleranceMa));
        REQUIRE_THAT(saturation_at_zero, WithinAbs(expected_saturation_12, kCurrentToleranceMa));
      }
    }

    SECTION("When using different Vref and Rf") {
      SECTION("Should scale output according to Vref_mV / Rf_Ohms") {
        using ConverterAlt = domain::signal::processors::TiaCurrentConverter<1024, 16, 1000>;
        ConverterAlt converter_alt;
        float const adc_max = static_cast<float>((1ULL << 16) - 1);
        float const expected_saturation_alt = 1024.0f / 1000.0f;

        REQUIRE_THAT(converter_alt.Process(0.0f),
                     Catch::Matchers::WithinAbs(expected_saturation_alt, kCurrentToleranceMa));
        REQUIRE_THAT(converter_alt.Process(adc_max),
                     Catch::Matchers::WithinAbs(0.0f, kCurrentToleranceMa));
      }
    }
  }

  SECTION("Signal processor concept") {
    SECTION("When used as a signal processor") {
      SECTION("Should satisfy SignalProcessor concept") {
        REQUIRE(domain::signal::is_signal_processor<TestConverter>::value);
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called") {
      SECTION("Should not change subsequent Process output for same input") {
        TestConverter converter;
        float const before = converter.Process(1000.0f);
        converter.Reset();
        float const after = converter.Process(1000.0f);

        using Catch::Matchers::WithinAbs;
        REQUIRE_THAT(before, WithinAbs(after, kCurrentToleranceMa));
      }
    }
  }
}

#endif
