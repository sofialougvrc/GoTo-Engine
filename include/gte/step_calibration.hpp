#pragma once

#include "gte/coord_transform.hpp"

#include <cstdint>

namespace gte {

struct AxisStepCalibration {
    std::uint32_t motor_steps_per_revolution;
    double gearbox_ratio;
    std::uint16_t microsteps_per_full_step;
    double external_gear_reduction;

    double stepsPerOutputRevolution() const;
    double stepsPerDegree() const;
    std::int64_t degreesToSteps(double delta_deg) const;
};

struct MountStepCalibration {
    AxisStepCalibration altitude;
    AxisStepCalibration azimuth;
};

struct AxisStepDelta {
    std::int64_t altitude_steps;
    std::int64_t azimuth_steps;
};

AxisStepDelta horizontalDeltaToSteps(
    HorizontalCoord delta,
    const MountStepCalibration& calibration);

} // namespace gte
