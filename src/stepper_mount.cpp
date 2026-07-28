#include "gte/stepper_mount.hpp"

namespace gte {

StepperMount::StepperMount(StepperAxis& altitude_axis, StepperAxis& azimuth_axis)
    : altitude_axis_(altitude_axis),
      azimuth_axis_(azimuth_axis) {}

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

} // namespace gte
