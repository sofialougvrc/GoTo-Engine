#include "gte/gpio_interface.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

bool expectTrue(const std::string& name, bool condition) {
    if (condition) {
        return true;
    }

    std::cerr << name << " failed\n";
    return false;
}

bool expectCall(
    const std::string& name,
    const gte::GpioCall& call,
    gte::GpioCallType type,
    int pin,
    std::optional<gte::GpioPinMode> mode,
    std::optional<bool> value) {
    bool ok = true;
    ok &= expectTrue(name + " type", call.type == type);
    ok &= expectTrue(name + " pin", call.pin == pin);
    ok &= expectTrue(name + " mode", call.mode == mode);
    ok &= expectTrue(name + " value", call.value == value);
    return ok;
}

bool expectTimestampsNondecreasing(
    const std::string& name,
    const std::vector<gte::GpioCall>& calls) {
    for (std::size_t i = 1; i < calls.size(); ++i) {
        if (calls[i].timestamp < calls[i - 1].timestamp) {
            std::cerr << name << " failed at call " << i << '\n';
            return false;
        }
    }
    return true;
}

} // namespace

int main() {
    bool ok = true;

    gte::FakeGpio gpio;
    gpio.setMode(17, gte::GpioPinMode::Output);
    gpio.writeDigitalPin(17, true);
    gpio.writeDigitalPin(17, false);

    const auto& setup_calls = gpio.calls();
    ok &= expectTrue("Setup scenario logged three calls", setup_calls.size() == 3);
    if (setup_calls.size() == 3) {
        ok &= expectCall(
            "setMode output",
            setup_calls[0],
            gte::GpioCallType::SetMode,
            17,
            gte::GpioPinMode::Output,
            std::nullopt);
        ok &= expectCall(
            "write high",
            setup_calls[1],
            gte::GpioCallType::WriteDigital,
            17,
            std::nullopt,
            true);
        ok &= expectCall(
            "write low",
            setup_calls[2],
            gte::GpioCallType::WriteDigital,
            17,
            std::nullopt,
            false);
        ok &= expectTimestampsNondecreasing("Setup timestamps", setup_calls);
    }

    gpio.clearCalls();
    constexpr int kStepPin = 23;
    constexpr int kPulseCount = 4;
    for (int i = 0; i < kPulseCount; ++i) {
        gpio.writeDigitalPin(kStepPin, true);
        gpio.writeDigitalPin(kStepPin, false);
    }

    const auto& pulse_calls = gpio.calls();
    ok &= expectTrue("Step pulse scenario logged two writes per pulse", pulse_calls.size() == kPulseCount * 2);
    for (int i = 0; i < kPulseCount && pulse_calls.size() == kPulseCount * 2; ++i) {
        ok &= expectCall(
            "pulse high " + std::to_string(i),
            pulse_calls[static_cast<std::size_t>(i * 2)],
            gte::GpioCallType::WriteDigital,
            kStepPin,
            std::nullopt,
            true);
        ok &= expectCall(
            "pulse low " + std::to_string(i),
            pulse_calls[static_cast<std::size_t>(i * 2 + 1)],
            gte::GpioCallType::WriteDigital,
            kStepPin,
            std::nullopt,
            false);
    }
    ok &= expectTimestampsNondecreasing("Pulse timestamps", pulse_calls);

    gpio.clearCalls();
    gpio.setMode(24, gte::GpioPinMode::Input);
    gpio.setPinValue(24, true);
    const bool read_high = gpio.readDigitalPin(24);
    gpio.setPinValue(24, false);
    const bool read_low = gpio.readDigitalPin(24);

    const auto& read_calls = gpio.calls();
    ok &= expectTrue("Read returns configured high value", read_high);
    ok &= expectTrue("Read returns configured low value", !read_low);
    ok &= expectTrue("Read scenario logged mode plus two reads", read_calls.size() == 3);
    if (read_calls.size() == 3) {
        ok &= expectCall(
            "input mode",
            read_calls[0],
            gte::GpioCallType::SetMode,
            24,
            gte::GpioPinMode::Input,
            std::nullopt);
        ok &= expectCall(
            "read high",
            read_calls[1],
            gte::GpioCallType::ReadDigital,
            24,
            std::nullopt,
            true);
        ok &= expectCall(
            "read low",
            read_calls[2],
            gte::GpioCallType::ReadDigital,
            24,
            std::nullopt,
            false);
    }

    if (ok) {
        std::cout << "All GPIO tests passed\n";
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
