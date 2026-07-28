#pragma once

#include "gte/gpio_interface.hpp"

#include <cstdint>

namespace gte {

struct StepperAxisConfig {
    int step_pin;
    int direction_pin;
    int enable_pin;
    double steps_per_degree;
    bool enable_active_low = true;
    bool positive_direction_high = true;
};

class StepperAxis {
public:
    StepperAxis(GpioInterface& gpio, StepperAxisConfig config);

    void initialize();
    void enable();
    void disable();
    bool enabled() const;

    void moveSteps(std::int64_t delta_steps);
    void moveToStep(std::int64_t target_step);
    void moveToAngleDeg(double target_angle_deg);

    std::int64_t currentStep() const;
    double currentAngleDeg() const;
    const StepperAxisConfig& config() const;

private:
    bool enablePinValue(bool enabled) const;
    bool directionPinValue(std::int64_t delta_steps) const;
    std::int64_t angleToStep(double angle_deg) const;
    void pulseOnce();

    GpioInterface& gpio_;
    StepperAxisConfig config_;
    std::int64_t current_step_ = 0;
    bool enabled_ = false;
    bool initialized_ = false;
};

} // namespace gte
