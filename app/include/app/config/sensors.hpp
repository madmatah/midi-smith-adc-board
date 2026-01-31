#pragma once

#include <cstddef>
#include <cstdint>

namespace app::config_sensors {

constexpr std::uint8_t kSensorIds[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                                       12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
constexpr std::size_t kSensorCount = sizeof(kSensorIds) / sizeof(kSensorIds[0]);

// --- ADC12 Mapping (Dual Simultaneous) ---
// The DMA buffer contains 32-bit words where:
// Low 16 bits  = ADC1 (Master)
// High 16 bits = ADC2 (Slave)
//
// Array order: [Rank1_ADC1, Rank1_ADC2, Rank2_ADC1, Rank2_ADC2, ...]
constexpr std::uint8_t kAdc12SensorIds[] = {
    1,  2,   // Rank 1: Master (SEN1),  Slave (SEN2)
    3,  4,   // Rank 2: Master (SEN3),  Slave (SEN4)
    5,  6,   // Rank 3: Master (SEN5),  Slave (SEN6)
    7,  8,   // Rank 4: Master (SEN7),  Slave (SEN8)
    9,  10,  // Rank 5: Master (SEN9),  Slave (SEN10)
    12, 13,  // Rank 6: Master (SEN12), Slave (SEN13)
    11, 14   // Rank 7: Master (SEN11), Slave (SEN14)
};
constexpr std::size_t kAdc12ChannelCount = sizeof(kAdc12SensorIds) / sizeof(kAdc12SensorIds[0]);

// --- ADC3 Mapping (Single Mode) ---
// Simple linear sequence from Rank 1 to Rank 8
constexpr std::uint8_t kAdc3SensorIds[] = {15, 16, 17, 18, 19, 20, 21, 22};
constexpr std::size_t kAdc3ChannelCount = sizeof(kAdc3SensorIds) / sizeof(kAdc3SensorIds[0]);

}  // namespace app::config_sensors
