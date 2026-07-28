#include "gte/gpio_interface.hpp"

namespace gte {

void FakeGpio::setMode(int pin, GpioPinMode mode) {
    log(GpioCallType::SetMode, pin, mode, std::nullopt);
}

void FakeGpio::writeDigitalPin(int pin, bool value) {
    pin_values_[pin] = value;
    log(GpioCallType::WriteDigital, pin, std::nullopt, value);
}

bool FakeGpio::readDigitalPin(int pin) {
    const auto it = pin_values_.find(pin);
    const bool value = it != pin_values_.end() ? it->second : false;
    log(GpioCallType::ReadDigital, pin, std::nullopt, value);
    return value;
}

void FakeGpio::setPinValue(int pin, bool value) {
    pin_values_[pin] = value;
}

const std::vector<GpioCall>& FakeGpio::calls() const {
    return calls_;
}

void FakeGpio::clearCalls() {
    calls_.clear();
}

void FakeGpio::log(
    GpioCallType type,
    int pin,
    std::optional<GpioPinMode> mode,
    std::optional<bool> value) {
    calls_.push_back({
        type,
        pin,
        mode,
        value,
        std::chrono::steady_clock::now(),
    });
}

} // namespace gte
