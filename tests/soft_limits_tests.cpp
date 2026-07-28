#include "gte/soft_limits.hpp"
#include "gte/stepper_mount.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

bool expectTrue(const std::string& name, bool condition) {
    if (condition) {
        return true;
    }

    std::cerr << name << " failed\n";
    return false;
}

bool expectNear(const std::string& name, double actual, double expected, double tolerance) {
    const double error = std::abs(actual - expected);
    if (error <= tolerance) {
        return true;
    }

    std::cerr << name << " failed: expected " << expected
              << ", got " << actual << ", error " << error << '\n';
    return false;
}

template <typename Callable>
bool expectOutOfRange(const std::string& name, Callable callable) {
    try {
        callable();
    } catch (const std::out_of_range&) {
        return true;
    } catch (...) {
        std::cerr << name << " failed: threw unexpected exception type\n";
        return false;
    }

    std::cerr << name << " failed: did not throw\n";
    return false;
}

std::size_t countWrites(
    const std::vector<gte::GpioCall>& calls,
    int pin,
    bool value) {
    std::size_t count = 0;
    for (const auto& call : calls) {
        if (call.type == gte::GpioCallType::WriteDigital &&
            call.pin == pin &&
            call.value == value) {
            ++count;
        }
    }
    return count;
}

} // namespace

int main() {
    bool ok = true;

    const gte::MountStepCalibration calibration{
        .altitude = {
            .motor_steps_per_revolution = 200,
            .gearbox_ratio = 1.0,
            .microsteps_per_full_step = 1,
            .external_gear_reduction = 18.0,
        },
        .azimuth = {
            .motor_steps_per_revolution = 200,
            .gearbox_ratio = 1.0,
            .microsteps_per_full_step = 1,
            .external_gear_reduction = 9.0,
        },
    };

    const gte::MountSoftLimits limits = gte::softLimitsFromDegrees(
        {
            .altitude = {.min_deg = 0.0, .max_deg = 5.0},
            .azimuth = {.min_deg = -2.0, .max_deg = 3.0},
        },
        calibration);

    ok &= expectTrue("Altitude limit min step", limits.altitude.min_step == 0);
    ok &= expectTrue("Altitude limit max step", limits.altitude.max_step == 50);
    ok &= expectTrue("Azimuth limit min step", limits.azimuth.min_step == -10);
    ok &= expectTrue("Azimuth limit max step", limits.azimuth.max_step == 15);
    ok &= expectTrue("Limits contain inside target", limits.contains({.altitude_step = 20, .azimuth_step = 5}));
    ok &= expectTrue("Limits reject outside target", !limits.contains({.altitude_step = 51, .azimuth_step = 5}));

    gte::FakeGpio gpio;
    gte::StepperMount mount(gpio, {
        .altitude_pins = {
            .step_pin = 20,
            .direction_pin = 21,
            .enable_pin = 22,
        },
        .azimuth_pins = {
            .step_pin = 30,
            .direction_pin = 31,
            .enable_pin = 32,
        },
        .calibration = calibration,
        .tracking_update_period = std::chrono::duration<double>{0.1},
        .pulse_width = std::chrono::microseconds{0},
        .soft_limits = limits,
    });

    mount.initialize();
    gpio.clearCalls();

    mount.slewTo(2.0, 1.0);
    const auto normal_calls = gpio.calls();
    ok &= expectTrue("Normal move altitude target", mount.currentStepPosition().altitude_step == 20);
    ok &= expectTrue("Normal move azimuth target", mount.currentStepPosition().azimuth_step == 5);
    ok &= expectNear("Normal move altitude degrees", mount.currentPosition().alt_deg, 2.0, 1.0e-12);
    ok &= expectNear("Normal move azimuth degrees", mount.currentPosition().az_deg, 1.0, 1.0e-12);
    ok &= expectTrue("Normal move altitude pulses", countWrites(normal_calls, 20, true) == 20);
    ok &= expectTrue("Normal move azimuth pulses", countWrites(normal_calls, 30, true) == 5);

    gpio.clearCalls();
    mount.slewTo(5.0, -2.0);
    const auto boundary_calls = gpio.calls();
    ok &= expectTrue("Boundary move altitude target", mount.currentStepPosition().altitude_step == 50);
    ok &= expectTrue("Boundary move azimuth target", mount.currentStepPosition().azimuth_step == -10);
    ok &= expectNear("Boundary move altitude degrees", mount.currentPosition().alt_deg, 5.0, 1.0e-12);
    ok &= expectNear("Boundary move azimuth degrees", mount.currentPosition().az_deg, -2.0, 1.0e-12);
    ok &= expectTrue("Boundary move altitude pulses", countWrites(boundary_calls, 20, true) == 30);
    ok &= expectTrue("Boundary move azimuth pulses", countWrites(boundary_calls, 30, true) == 15);
    ok &= expectTrue("Boundary move azimuth negative direction", countWrites(boundary_calls, 31, false) == 1);

    gpio.clearCalls();
    ok &= expectOutOfRange("Rejects altitude above soft limit", [&mount] {
        mount.slewTo(5.1, -1.0);
    });
    ok &= expectTrue("Rejected altitude move emits no GPIO", gpio.calls().empty());
    ok &= expectTrue("Rejected altitude move preserves altitude step", mount.currentStepPosition().altitude_step == 50);
    ok &= expectTrue("Rejected altitude move preserves azimuth step", mount.currentStepPosition().azimuth_step == -10);

    gpio.clearCalls();
    ok &= expectOutOfRange("Rejects azimuth below soft limit", [&mount] {
        mount.slewTo(4.0, -2.2);
    });
    ok &= expectTrue("Rejected azimuth move emits no GPIO", gpio.calls().empty());
    ok &= expectTrue("Rejected azimuth move preserves altitude step", mount.currentStepPosition().altitude_step == 50);
    ok &= expectTrue("Rejected azimuth move preserves azimuth step", mount.currentStepPosition().azimuth_step == -10);

    gpio.clearCalls();
    ok &= expectOutOfRange("Rejects tracking update outside soft limit", [&mount] {
        mount.track(4.0, 3.2);
    });
    mount.scheduler().stop();
    ok &= expectTrue("Rejected tracking update emits no GPIO", gpio.calls().empty());

    if (ok) {
        std::cout << "All soft limits tests passed\n";
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
