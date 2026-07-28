#pragma once

#include "gte/gpio_interface.hpp"

#include <map>

namespace gte {

class LgpioGpio : public GpioInterface {
public:
    explicit LgpioGpio(int gpio_chip = 4);
    ~LgpioGpio() override;

    LgpioGpio(const LgpioGpio&) = delete;
    LgpioGpio& operator=(const LgpioGpio&) = delete;
    LgpioGpio(LgpioGpio&& other) noexcept;
    LgpioGpio& operator=(LgpioGpio&& other) noexcept;

    void setMode(int pin, GpioPinMode mode) override;
    void writeDigitalPin(int pin, bool value) override;
    bool readDigitalPin(int pin) override;

    int gpioChip() const;
    int handle() const;

private:
    void close();
    void ensureOpen(const char* action) const;

    int gpio_chip_ = -1;
    int handle_ = -1;
    std::map<int, GpioPinMode> claimed_modes_;
};

} // namespace gte
