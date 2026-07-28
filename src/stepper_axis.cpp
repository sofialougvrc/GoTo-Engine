#include "gte/stepper_axis.hpp"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace gte {

StepperAxis::StepperAxis(GpioInterface& gpio, StepperAxisConfig config)
    : gpio_(gpio),
      config_(config) {
    if (config_.steps_per_degree <= 0.0) {
        throw std::invalid_argument("StepperAxis steps_per_degree must be positive");
    }
}

void StepperAxis::initialize() {
    gpio_.setMode(config_.step_pin, GpioPinMode::Output);
    gpio_.setMode(config_.direction_pin, GpioPinMode::Output);
    gpio_.setMode(config_.enable_pin, GpioPinMode::Output);
    gpio_.writeDigitalPin(config_.step_pin, false);
    disable();
    initialized_ = true;
}

void StepperAxis::enable() {
    gpio_.writeDigitalPin(config_.enable_pin, enablePinValue(true));
    enabled_ = true;
}

void StepperAxis::disable() {
    gpio_.writeDigitalPin(config_.enable_pin, enablePinValue(false));
    enabled_ = false;
}

bool StepperAxis::enabled() const {
    return enabled_;
}

void StepperAxis::moveSteps(std::int64_t delta_steps) {
    if (delta_steps == 0) {
        return;
    }

    if (!initialized_) {
        initialize();
    }
    if (!enabled_) {
        enable();
    }

    gpio_.writeDigitalPin(config_.direction_pin, directionPinValue(delta_steps));

    const std::uint64_t steps =
        static_cast<std::uint64_t>(delta_steps > 0 ? delta_steps : -delta_steps);
    for (std::uint64_t i = 0; i < steps; ++i) {
        pulseOnce();
    }

    current_step_ += delta_steps;
}

void StepperAxis::moveToStep(std::int64_t target_step) {
    moveSteps(target_step - current_step_);
}

void StepperAxis::moveToAngleDeg(double target_angle_deg) {
    moveToStep(angleToStep(target_angle_deg));
}

std::int64_t StepperAxis::currentStep() const {
    return current_step_;
}

double StepperAxis::currentAngleDeg() const {
    return static_cast<double>(current_step_) / config_.steps_per_degree;
}

const StepperAxisConfig& StepperAxis::config() const {
    return config_;
}

bool StepperAxis::enablePinValue(bool enabled) const {
    return config_.enable_active_low ? !enabled : enabled;
}

bool StepperAxis::directionPinValue(std::int64_t delta_steps) const {
    const bool positive = delta_steps > 0;
    return positive == config_.positive_direction_high;
}

std::int64_t StepperAxis::angleToStep(double angle_deg) const {
    return static_cast<std::int64_t>(std::llround(angle_deg * config_.steps_per_degree));
}

void StepperAxis::pulseOnce() {
    gpio_.writeDigitalPin(config_.step_pin, true);
    gpio_.writeDigitalPin(config_.step_pin, false);
}

} // namespace gte
