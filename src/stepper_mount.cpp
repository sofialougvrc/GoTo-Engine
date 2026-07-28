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
      scheduler_(gpio, {
          .altitude_axis = makeAxisConfig(config_.altitude_pins, config_.calibration.altitude),
          .azimuth_axis = makeAxisConfig(config_.azimuth_pins, config_.calibration.azimuth),
          .pulse_width = config_.pulse_width,
      }) {}

void StepperMount::initialize() {
    scheduler_.initialize();
    scheduler_.start();
}

void StepperMount::enable() {
    scheduler_.enable();
}

void StepperMount::disable() {
    scheduler_.disable();
}

void StepperMount::slewTo(double alt_deg, double az_deg) {
    scheduler_.moveToStepTargetsBlocking(angleTargets(alt_deg, az_deg));
}

void StepperMount::track(double alt_deg, double az_deg) {
    if (config_.tracking_update_period.count() <= 0.0) {
        slewTo(alt_deg, az_deg);
        return;
    }

    const StepSchedulerTargets targets = angleTargets(alt_deg, az_deg);
    scheduler_.start();
    scheduler_.moveToTargetsAtRates(targets, ratesForTarget(targets));
}

HorizontalCoord StepperMount::currentPosition() const {
    const StepSchedulerTargets steps = scheduler_.currentSteps();
    return {
        static_cast<double>(steps.altitude_step) / config_.calibration.altitude.stepsPerDegree(),
        static_cast<double>(steps.azimuth_step) / config_.calibration.azimuth.stepsPerDegree(),
    };
}

StepSchedulerTargets StepperMount::currentStepPosition() const {
    return scheduler_.currentSteps();
}

const StepperMountConfig& StepperMount::config() const {
    return config_;
}

StepScheduler& StepperMount::scheduler() {
    return scheduler_;
}

const StepScheduler& StepperMount::scheduler() const {
    return scheduler_;
}

StepSchedulerTargets StepperMount::angleTargets(double alt_deg, double az_deg) const {
    return {
        config_.calibration.altitude.degreesToSteps(alt_deg),
        config_.calibration.azimuth.degreesToSteps(az_deg),
    };
}

StepSchedulerRates StepperMount::ratesForTarget(StepSchedulerTargets targets) const {
    const StepSchedulerTargets current = scheduler_.currentSteps();
    const double seconds = config_.tracking_update_period.count();
    return {
        static_cast<double>(targets.altitude_step - current.altitude_step) / seconds,
        static_cast<double>(targets.azimuth_step - current.azimuth_step) / seconds,
    };
}

} // namespace gte
