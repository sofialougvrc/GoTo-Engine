#pragma once

#include "gte/uart_interface.hpp"

#include <cstdint>
#include <vector>

namespace gte {

struct Tmc2209Config {
    std::uint8_t slave_address = 0;
    std::uint16_t microsteps_per_full_step = 16;
    std::uint8_t run_current = 20;
    std::uint8_t hold_current = 10;
    std::uint8_t hold_delay = 6;
    std::uint8_t stallguard_threshold = 0;
    bool interpolate_to_256 = true;
};

class Tmc2209Configurator {
public:
    explicit Tmc2209Configurator(UartInterface& uart);

    void applyConfig(const Tmc2209Config& config);

    static std::vector<std::vector<std::uint8_t>> serializeConfig(const Tmc2209Config& config);
    static std::vector<std::uint8_t> writeDatagram(
        std::uint8_t slave_address,
        std::uint8_t register_address,
        std::uint32_t value);

    static std::uint32_t iholdIrunRegisterValue(const Tmc2209Config& config);
    static std::uint32_t chopconfRegisterValue(const Tmc2209Config& config);
    static std::uint32_t sgthrsRegisterValue(const Tmc2209Config& config);

private:
    UartInterface& uart_;
};

} // namespace gte
