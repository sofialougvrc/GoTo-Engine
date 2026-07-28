#include "gte/stepper_mount.hpp"

namespace gte {
namespace {

StepperAxisConfig makeAxisConfig(
    const StepperMountAxisPins& pins,
    const AxisStepCalibration& calibration) {
    return {
        .step_pin = pins.step_pin,
        .direction_pin = pins.direction_pin,
        .enable_pin = pins.enable_pin,
        .steps_per_degree = calibration.stepsPerDegree(),
        .enable_active_low = pins.enable_active_low,
        .positive_direction_high = pins.positive_direction_high,
    };
}

} // namespace

StepperMount::StepperMount(GpioInterface& gpio, StepperMountConfig config)
    : config_(config),
      altitude_axis_(gpio, makeAxisConfig(config_.altitude_pins, config_.calibration.altitude)),
      azimuth_axis_(gpio, makeAxisConfig(config_.azimuth_pins, config_.calibration.azimuth)) {}

void StepperMount::initialize() {
    altitude_axis_.initialize();
    azimuth_axis_.initialize();
}

void StepperMount::enable() {
    altitude_axis_.enable();
    azimuth_axis_.enable();
}

void StepperMount::disable() {
    altitude_axis_.disable();
    azimuth_axis_.disable();
}

void StepperMount::slewTo(double alt_deg, double az_deg) {
    altitude_axis_.moveToAngleDeg(alt_deg);
    azimuth_axis_.moveToAngleDeg(az_deg);
}

HorizontalCoord StepperMount::currentPosition() const {
    return {
        altitude_axis_.currentAngleDeg(),
        azimuth_axis_.currentAngleDeg(),
    };
}

const StepperAxis& StepperMount::altitudeAxis() const {
    return altitude_axis_;
}

const StepperAxis& StepperMount::azimuthAxis() const {
    return azimuth_axis_;
}

const StepperMountConfig& StepperMount::config() const {
    return config_;
}

} // namespace gte
