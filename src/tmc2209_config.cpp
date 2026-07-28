#include "gte/tmc2209_config.hpp"

#include <stdexcept>

namespace gte {
namespace {

constexpr std::uint8_t kSyncByte = 0x05;
constexpr std::uint8_t kWriteBit = 0x80;
constexpr std::uint8_t kRegisterIholdIrun = 0x10;
constexpr std::uint8_t kRegisterSgthrs = 0x40;
constexpr std::uint8_t kRegisterChopconf = 0x6C;
constexpr std::uint32_t kChopconfBase = 0x00000053;
constexpr std::uint32_t kChopconfIntpolBit = 1UL << 28;
constexpr std::uint32_t kChopconfMresMask = 0x0FUL << 24;

std::uint8_t crc8Atm(const std::vector<std::uint8_t>& bytes_without_crc) {
    std::uint8_t crc = 0;

    for (std::uint8_t byte : bytes_without_crc) {
        std::uint8_t current_byte = byte;
        for (int bit = 0; bit < 8; ++bit) {
            if (((crc >> 7) ^ (current_byte & 0x01)) != 0) {
                crc = static_cast<std::uint8_t>((crc << 1) ^ 0x07);
            } else {
                crc = static_cast<std::uint8_t>(crc << 1);
            }
            current_byte >>= 1;
        }
    }

    return crc;
}

std::uint8_t microstepsToMres(std::uint16_t microsteps_per_full_step) {
    switch (microsteps_per_full_step) {
        case 256:
            return 0;
        case 128:
            return 1;
        case 64:
            return 2;
        case 32:
            return 3;
        case 16:
            return 4;
        case 8:
            return 5;
        case 4:
            return 6;
        case 2:
            return 7;
        case 1:
            return 8;
        default:
            throw std::invalid_argument("Unsupported TMC2209 microstep setting");
    }
}

void validateCurrentScale(std::uint8_t value, const char* name) {
    if (value > 31) {
        throw std::invalid_argument(std::string("TMC2209 ") + name + " must be in [0, 31]");
    }
}

} // namespace

Tmc2209Configurator::Tmc2209Configurator(UartInterface& uart)
    : uart_(uart) {}

void Tmc2209Configurator::applyConfig(const Tmc2209Config& config) {
    for (const auto& datagram : serializeConfig(config)) {
        uart_.writeBytes(datagram);
    }
}

std::vector<std::vector<std::uint8_t>> Tmc2209Configurator::serializeConfig(
    const Tmc2209Config& config) {
    return {
        writeDatagram(config.slave_address, kRegisterIholdIrun, iholdIrunRegisterValue(config)),
        writeDatagram(config.slave_address, kRegisterChopconf, chopconfRegisterValue(config)),
        writeDatagram(config.slave_address, kRegisterSgthrs, sgthrsRegisterValue(config)),
    };
}

std::vector<std::uint8_t> Tmc2209Configurator::writeDatagram(
    std::uint8_t slave_address,
    std::uint8_t register_address,
    std::uint32_t value) {
    if (slave_address > 3) {
        throw std::invalid_argument("TMC2209 slave address must be in [0, 3]");
    }

    std::vector<std::uint8_t> datagram = {
        kSyncByte,
        slave_address,
        static_cast<std::uint8_t>(register_address | kWriteBit),
        static_cast<std::uint8_t>((value >> 24) & 0xFF),
        static_cast<std::uint8_t>((value >> 16) & 0xFF),
        static_cast<std::uint8_t>((value >> 8) & 0xFF),
        static_cast<std::uint8_t>(value & 0xFF),
    };
    datagram.push_back(crc8Atm(datagram));
    return datagram;
}

std::uint32_t Tmc2209Configurator::iholdIrunRegisterValue(const Tmc2209Config& config) {
    validateCurrentScale(config.hold_current, "hold_current");
    validateCurrentScale(config.run_current, "run_current");
    if (config.hold_delay > 15) {
        throw std::invalid_argument("TMC2209 hold_delay must be in [0, 15]");
    }

    return
        (static_cast<std::uint32_t>(config.hold_delay) << 16) |
        (static_cast<std::uint32_t>(config.run_current) << 8) |
        static_cast<std::uint32_t>(config.hold_current);
}

std::uint32_t Tmc2209Configurator::chopconfRegisterValue(const Tmc2209Config& config) {
    std::uint32_t value = kChopconfBase;
    if (config.interpolate_to_256) {
        value |= kChopconfIntpolBit;
    }

    value &= ~kChopconfMresMask;
    value |= static_cast<std::uint32_t>(microstepsToMres(config.microsteps_per_full_step)) << 24;
    return value;
}

std::uint32_t Tmc2209Configurator::sgthrsRegisterValue(const Tmc2209Config& config) {
    return config.stallguard_threshold;
}

} // namespace gte
