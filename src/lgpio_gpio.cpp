#include "gte/lgpio_gpio.hpp"

#include <stdexcept>
#include <string>
#include <utility>

#if defined(GTE_ENABLE_LGPIO)
#include <lgpio.h>
#endif

namespace gte {
namespace {

#if defined(GTE_ENABLE_LGPIO)
std::runtime_error lgpioError(const std::string& action, int result) {
    return std::runtime_error(
        action + " failed: " + lguErrorText(result) + " (" + std::to_string(result) + ")");
}

void checkLgpio(const std::string& action, int result) {
    if (result < 0) {
        throw lgpioError(action, result);
    }
}
#endif

std::runtime_error unavailableError() {
    return std::runtime_error(
        "LgpioGpio requires Linux with lgpio installed and GTE_ENABLE_LGPIO enabled");
}

} // namespace

LgpioGpio::LgpioGpio(int gpio_chip)
    : gpio_chip_(gpio_chip) {
#if defined(GTE_ENABLE_LGPIO)
    handle_ = lgGpiochipOpen(gpio_chip_);
    if (handle_ < 0) {
        throw lgpioError("lgGpiochipOpen", handle_);
    }
#else
    (void)gpio_chip_;
    throw unavailableError();
#endif
}

LgpioGpio::~LgpioGpio() {
    close();
}

LgpioGpio::LgpioGpio(LgpioGpio&& other) noexcept
    : gpio_chip_(other.gpio_chip_),
      handle_(other.handle_),
      claimed_modes_(std::move(other.claimed_modes_)) {
    other.gpio_chip_ = -1;
    other.handle_ = -1;
}

LgpioGpio& LgpioGpio::operator=(LgpioGpio&& other) noexcept {
    if (this != &other) {
        close();
        gpio_chip_ = other.gpio_chip_;
        handle_ = other.handle_;
        claimed_modes_ = std::move(other.claimed_modes_);
        other.gpio_chip_ = -1;
        other.handle_ = -1;
    }
    return *this;
}

void LgpioGpio::setMode(int pin, GpioPinMode mode) {
#if defined(GTE_ENABLE_LGPIO)
    ensureOpen("setMode");

    const auto existing = claimed_modes_.find(pin);
    if (existing != claimed_modes_.end() && existing->second == mode) {
        return;
    }
    if (existing != claimed_modes_.end()) {
        checkLgpio("lgGpioFree", lgGpioFree(handle_, pin));
    }

    if (mode == GpioPinMode::Output) {
        checkLgpio("lgGpioClaimOutput", lgGpioClaimOutput(handle_, 0, pin, 0));
    } else {
        checkLgpio("lgGpioClaimInput", lgGpioClaimInput(handle_, 0, pin));
    }

    claimed_modes_[pin] = mode;
#else
    (void)pin;
    (void)mode;
    throw unavailableError();
#endif
}

void LgpioGpio::writeDigitalPin(int pin, bool value) {
#if defined(GTE_ENABLE_LGPIO)
    ensureOpen("writeDigitalPin");
    checkLgpio("lgGpioWrite", lgGpioWrite(handle_, pin, value ? 1 : 0));
#else
    (void)pin;
    (void)value;
    throw unavailableError();
#endif
}

bool LgpioGpio::readDigitalPin(int pin) {
#if defined(GTE_ENABLE_LGPIO)
    ensureOpen("readDigitalPin");
    const int result = lgGpioRead(handle_, pin);
    checkLgpio("lgGpioRead", result);
    return result != 0;
#else
    (void)pin;
    throw unavailableError();
#endif
}

int LgpioGpio::gpioChip() const {
    return gpio_chip_;
}

int LgpioGpio::handle() const {
    return handle_;
}

void LgpioGpio::close() {
#if defined(GTE_ENABLE_LGPIO)
    if (handle_ >= 0) {
        (void)lgGpiochipClose(handle_);
        handle_ = -1;
    }
#else
    handle_ = -1;
#endif
    claimed_modes_.clear();
}

void LgpioGpio::ensureOpen(const char* action) const {
    if (handle_ < 0) {
        throw std::runtime_error(std::string("LgpioGpio handle is not open for ") + action);
    }
}

} // namespace gte
