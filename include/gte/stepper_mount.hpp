#pragma once

#include "gte/gpio_interface.hpp"
#include "gte/mount.hpp"
#include "gte/step_calibration.hpp"
#include "gte/step_scheduler.hpp"
#include "gte/stepper_axis.hpp"

#include <chrono>

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
    std::chrono::duration<double> tracking_update_period{1.0};
    std::chrono::microseconds pulse_width{50};
};

class StepperMount : public IMountDriver {
public:
    StepperMount(GpioInterface& gpio, StepperMountConfig config);

    void initialize();
    void enable();
    void disable();

    void slewTo(double alt_deg, double az_deg) override;
    void track(double alt_deg, double az_deg) override;
    HorizontalCoord currentPosition() const override;

    StepSchedulerTargets currentStepPosition() const;
    const StepperMountConfig& config() const;
    StepScheduler& scheduler();
    const StepScheduler& scheduler() const;

private:
    StepSchedulerTargets angleTargets(double alt_deg, double az_deg) const;
    StepSchedulerRates ratesForTarget(StepSchedulerTargets targets) const;

    StepperMountConfig config_;
    StepScheduler scheduler_;
};

} // namespace gte
