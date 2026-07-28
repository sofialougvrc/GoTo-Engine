#include "gte/tmc2209_config.hpp"

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string bytesToString(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream out;
    out << std::hex << std::uppercase << std::setfill('0');
    for (std::uint8_t byte : bytes) {
        out << "0x" << std::setw(2) << static_cast<int>(byte) << ' ';
    }
    return out.str();
}

bool expectTrue(const std::string& name, bool condition) {
    if (condition) {
        return true;
    }

    std::cerr << name << " failed\n";
    return false;
}

bool expectEqual(
    const std::string& name,
    const std::vector<std::uint8_t>& actual,
    const std::vector<std::uint8_t>& expected) {
    if (actual == expected) {
        return true;
    }

    std::cerr << name << " failed:\n"
              << "  expected: " << bytesToString(expected) << '\n'
              << "  actual:   " << bytesToString(actual) << '\n';
    return false;
}

template <typename Callable>
bool expectThrows(const std::string& name, Callable callable) {
    try {
        callable();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (...) {
        std::cerr << name << " failed: threw unexpected exception type\n";
        return false;
    }

    std::cerr << name << " failed: did not throw\n";
    return false;
}

} // namespace

int main() {
    bool ok = true;

    const gte::Tmc2209Config config{
        .slave_address = 0,
        .microsteps_per_full_step = 16,
        .run_current = 20,
        .hold_current = 10,
        .hold_delay = 6,
        .stallguard_threshold = 42,
        .interpolate_to_256 = true,
    };

    ok &= expectTrue(
        "IHOLD_IRUN register value",
        gte::Tmc2209Configurator::iholdIrunRegisterValue(config) == 0x0006140A);
    ok &= expectTrue(
        "CHOPCONF register value",
        gte::Tmc2209Configurator::chopconfRegisterValue(config) == 0x14000053);
    ok &= expectTrue(
        "SGTHRS register value",
        gte::Tmc2209Configurator::sgthrsRegisterValue(config) == 0x0000002A);

    const auto datagrams = gte::Tmc2209Configurator::serializeConfig(config);
    ok &= expectTrue("Config serializes to three writes", datagrams.size() == 3);
    if (datagrams.size() == 3) {
        ok &= expectEqual(
            "IHOLD_IRUN datagram",
            datagrams[0],
            {0x05, 0x00, 0x90, 0x00, 0x06, 0x14, 0x0A, 0x24});
        ok &= expectEqual(
            "CHOPCONF datagram",
            datagrams[1],
            {0x05, 0x00, 0xEC, 0x14, 0x00, 0x00, 0x53, 0x52});
        ok &= expectEqual(
            "SGTHRS datagram",
            datagrams[2],
            {0x05, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x2A, 0x26});
    }

    gte::FakeUart uart;
    gte::Tmc2209Configurator configurator(uart);
    configurator.applyConfig(config);
    ok &= expectTrue("Fake UART captured three writes", uart.writes().size() == 3);
    if (uart.writes().size() == 3 && datagrams.size() == 3) {
        ok &= expectEqual("Fake UART write 0", uart.writes()[0].bytes, datagrams[0]);
        ok &= expectEqual("Fake UART write 1", uart.writes()[1].bytes, datagrams[1]);
        ok &= expectEqual("Fake UART write 2", uart.writes()[2].bytes, datagrams[2]);
        ok &= expectTrue(
            "Fake UART timestamps nondecreasing",
            uart.writes()[0].timestamp <= uart.writes()[1].timestamp &&
            uart.writes()[1].timestamp <= uart.writes()[2].timestamp);
    }

    const gte::Tmc2209Config alternate{
        .slave_address = 2,
        .microsteps_per_full_step = 32,
        .run_current = 16,
        .hold_current = 8,
        .hold_delay = 1,
        .stallguard_threshold = 7,
        .interpolate_to_256 = true,
    };
    const auto alternate_datagrams = gte::Tmc2209Configurator::serializeConfig(alternate);
    ok &= expectTrue("Alternate config serializes to three writes", alternate_datagrams.size() == 3);
    if (alternate_datagrams.size() == 3) {
        ok &= expectEqual(
            "Alternate IHOLD_IRUN datagram",
            alternate_datagrams[0],
            {0x05, 0x02, 0x90, 0x00, 0x01, 0x10, 0x08, 0xF5});
        ok &= expectEqual(
            "Alternate CHOPCONF datagram",
            alternate_datagrams[1],
            {0x05, 0x02, 0xEC, 0x13, 0x00, 0x00, 0x53, 0x40});
        ok &= expectEqual(
            "Alternate SGTHRS datagram",
            alternate_datagrams[2],
            {0x05, 0x02, 0xC0, 0x00, 0x00, 0x00, 0x07, 0x55});
    }

    ok &= expectThrows("Rejects unsupported microstep setting", [] {
        gte::Tmc2209Config bad_config;
        bad_config.microsteps_per_full_step = 3;
        (void)gte::Tmc2209Configurator::chopconfRegisterValue(bad_config);
    });
    ok &= expectThrows("Rejects invalid run current", [] {
        gte::Tmc2209Config bad_config;
        bad_config.run_current = 32;
        (void)gte::Tmc2209Configurator::iholdIrunRegisterValue(bad_config);
    });
    ok &= expectThrows("Rejects invalid slave address", [] {
        (void)gte::Tmc2209Configurator::writeDatagram(4, 0x10, 0);
    });

    if (ok) {
        std::cout << "All TMC2209 config tests passed\n";
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
