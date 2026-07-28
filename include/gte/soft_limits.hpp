#pragma once

#include "gte/coord_transform.hpp"
#include "gte/step_calibration.hpp"
#include "gte/step_scheduler.hpp"

#include <cstdint>

namespace gte {

struct AxisSoftLimit {
    std::int64_t min_step;
    std::int64_t max_step;

    bool contains(std::int64_t step) const;
    void validate(std::int64_t step, const char* axis_name) const;
};

struct MountSoftLimits {
    AxisSoftLimit altitude;
    AxisSoftLimit azimuth;

    bool contains(StepSchedulerTargets target) const;
    void validate(StepSchedulerTargets target) const;
};

struct AxisDegreeSoftLimit {
    double min_deg;
    double max_deg;
};

struct MountDegreeSoftLimits {
    AxisDegreeSoftLimit altitude;
    AxisDegreeSoftLimit azimuth;
};

MountSoftLimits softLimitsFromDegrees(
    MountDegreeSoftLimits degree_limits,
    const MountStepCalibration& calibration);

} // namespace gte
