#pragma once

#include "gte/gpio_interface.hpp"
#include "gte/mount.hpp"
#include "gte/step_calibration.hpp"
#include "gte/stepper_axis.hpp"

namespace gte {

struct StepperMountAxisPins {
    int step_pin;
    int direction_pin;
    int enable_pin;
    bool enable_active_low = true;
    bool positive_direction_high = true;
};

struct StepperMountConfig {
    StepperMountAxisPins altitude_pins;
    StepperMountAxisPins azimuth_pins;
    MountStepCalibration calibration;
};

class StepperMount : public IMountDriver {
public:
    StepperMount(GpioInterface& gpio, StepperMountConfig config);

    void initialize();
    void enable();
    void disable();

    void slewTo(double alt_deg, double az_deg) override;
    HorizontalCoord currentPosition() const override;

    const StepperAxis& altitudeAxis() const;
    const StepperAxis& azimuthAxis() const;
    const StepperMountConfig& config() const;

private:
    StepperMountConfig config_;
    StepperAxis altitude_axis_;
    StepperAxis azimuth_axis_;
};

} // namespace gte
