#pragma once

#include <chrono>
#include <map>
#include <optional>
#include <vector>

namespace gte {

enum class GpioPinMode {
    Input,
    Output,
};

enum class GpioCallType {
    SetMode,
    WriteDigital,
    ReadDigital,
};

struct GpioCall {
    GpioCallType type;
    int pin;
    std::optional<GpioPinMode> mode;
    std::optional<bool> value;
    std::chrono::steady_clock::time_point timestamp;
};

class GpioInterface {
public:
    virtual void setMode(int pin, GpioPinMode mode) = 0;
    virtual void writeDigitalPin(int pin, bool value) = 0;
    virtual bool readDigitalPin(int pin) = 0;
    virtual ~GpioInterface() = default;
};

class FakeGpio : public GpioInterface {
public:
    void setMode(int pin, GpioPinMode mode) override;
    void writeDigitalPin(int pin, bool value) override;
    bool readDigitalPin(int pin) override;

    void setPinValue(int pin, bool value);
    const std::vector<GpioCall>& calls() const;
    void clearCalls();

private:
    void log(GpioCallType type, int pin, std::optional<GpioPinMode> mode, std::optional<bool> value);

    std::vector<GpioCall> calls_;
    std::map<int, bool> pin_values_;
};

} // namespace gte
