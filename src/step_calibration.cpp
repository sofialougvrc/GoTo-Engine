#include "gte/step_calibration.hpp"

#include <cmath>
#include <stdexcept>

namespace gte {

double AxisStepCalibration::stepsPerOutputRevolution() const {
    if (motor_steps_per_revolution == 0) {
        throw std::invalid_argument("motor_steps_per_revolution must be positive");
    }
    if (gearbox_ratio <= 0.0) {
        throw std::invalid_argument("gearbox_ratio must be positive");
    }
    if (microsteps_per_full_step == 0) {
        throw std::invalid_argument("microsteps_per_full_step must be positive");
    }
    if (external_gear_reduction <= 0.0) {
        throw std::invalid_argument("external_gear_reduction must be positive");
    }

    return
        static_cast<double>(motor_steps_per_revolution) *
        static_cast<double>(microsteps_per_full_step) *
        gearbox_ratio *
        external_gear_reduction;
}

double AxisStepCalibration::stepsPerDegree() const {
    return stepsPerOutputRevolution() / 360.0;
}

std::int64_t AxisStepCalibration::degreesToSteps(double delta_deg) const {
    return static_cast<std::int64_t>(std::llround(delta_deg * stepsPerDegree()));
}

AxisStepDelta horizontalDeltaToSteps(
    HorizontalCoord delta,
    const MountStepCalibration& calibration) {
    return {
        calibration.altitude.degreesToSteps(delta.alt_deg),
        calibration.azimuth.degreesToSteps(delta.az_deg),
    };
}

} // namespace gte
