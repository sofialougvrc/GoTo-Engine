#pragma once

#include "gte/mount.hpp"
#include "gte/stepper_axis.hpp"

namespace gte {

class StepperMount : public IMountDriver {
public:
    StepperMount(StepperAxis& altitude_axis, StepperAxis& azimuth_axis);

    void initialize();
    void enable();
    void disable();

    void slewTo(double alt_deg, double az_deg) override;
    HorizontalCoord currentPosition() const override;

private:
    StepperAxis& altitude_axis_;
    StepperAxis& azimuth_axis_;
};

} // namespace gte
