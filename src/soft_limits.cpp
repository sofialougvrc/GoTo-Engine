#include "gte/soft_limits.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace gte {
namespace {

AxisSoftLimit axisSoftLimitFromDegrees(
    AxisDegreeSoftLimit degree_limit,
    const AxisStepCalibration& calibration) {
    const std::int64_t a = calibration.degreesToSteps(degree_limit.min_deg);
    const std::int64_t b = calibration.degreesToSteps(degree_limit.max_deg);
    return {
        std::min(a, b),
        std::max(a, b),
    };
}

} // namespace

bool AxisSoftLimit::contains(std::int64_t step) const {
    return step >= min_step && step <= max_step;
}

void AxisSoftLimit::validate(std::int64_t step, const char* axis_name) const {
    if (contains(step)) {
        return;
    }

    throw std::out_of_range(
        std::string(axis_name) + " target step " + std::to_string(step) +
        " is outside soft limits [" + std::to_string(min_step) +
        ", " + std::to_string(max_step) + "]");
}

bool MountSoftLimits::contains(StepSchedulerTargets target) const {
    return
        altitude.contains(target.altitude_step) &&
        azimuth.contains(target.azimuth_step);
}

void MountSoftLimits::validate(StepSchedulerTargets target) const {
    altitude.validate(target.altitude_step, "altitude");
    azimuth.validate(target.azimuth_step, "azimuth");
}

MountSoftLimits softLimitsFromDegrees(
    MountDegreeSoftLimits degree_limits,
    const MountStepCalibration& calibration) {
    return {
        axisSoftLimitFromDegrees(degree_limits.altitude, calibration.altitude),
        axisSoftLimitFromDegrees(degree_limits.azimuth, calibration.azimuth),
    };
}

} // namespace gte
