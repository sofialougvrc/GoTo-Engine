#include "gte/stepper_axis.hpp"
#include "gte/stepper_mount.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

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

bool expectCall(
    const std::string& name,
    const gte::GpioCall& call,
    gte::GpioCallType type,
    int pin,
    std::optional<gte::GpioPinMode> mode,
    std::optional<bool> value) {
    bool ok = true;
    ok &= expectTrue(name + " type", call.type == type);
    ok &= expectTrue(name + " pin", call.pin == pin);
    ok &= expectTrue(name + " mode", call.mode == mode);
    ok &= expectTrue(name + " value", call.value == value);
    return ok;
}

} // namespace

int main() {
    bool ok = true;

    gte::FakeGpio gpio;
    gte::StepperAxis axis(gpio, {
        .step_pin = 10,
        .direction_pin = 11,
        .enable_pin = 12,
        .steps_per_degree = 4.0,
    });

    axis.initialize();
    const auto& init_calls = gpio.calls();
    ok &= expectTrue("Stepper axis initialization logs five calls", init_calls.size() == 5);
    if (init_calls.size() == 5) {
        ok &= expectCall("step pin mode", init_calls[0], gte::GpioCallType::SetMode, 10, gte::GpioPinMode::Output, std::nullopt);
        ok &= expectCall("direction pin mode", init_calls[1], gte::GpioCallType::SetMode, 11, gte::GpioPinMode::Output, std::nullopt);
        ok &= expectCall("enable pin mode", init_calls[2], gte::GpioCallType::SetMode, 12, gte::GpioPinMode::Output, std::nullopt);
        ok &= expectCall("step pin starts low", init_calls[3], gte::GpioCallType::WriteDigital, 10, std::nullopt, false);
        ok &= expectCall("active-low driver starts disabled", init_calls[4], gte::GpioCallType::WriteDigital, 12, std::nullopt, true);
    }
    ok &= expectTrue("Axis starts disabled", !axis.enabled());

    gpio.clearCalls();
    axis.moveSteps(3);
    const auto& forward_calls = gpio.calls();
    ok &= expectTrue("Forward move enables, sets direction, and pulses", forward_calls.size() == 8);
    if (forward_calls.size() == 8) {
        ok &= expectCall("enable before forward move", forward_calls[0], gte::GpioCallType::WriteDigital, 12, std::nullopt, false);
        ok &= expectCall("forward direction", forward_calls[1], gte::GpioCallType::WriteDigital, 11, std::nullopt, true);
        for (int i = 0; i < 3; ++i) {
            ok &= expectCall(
                "forward pulse high " + std::to_string(i),
                forward_calls[static_cast<std::size_t>(2 + i * 2)],
                gte::GpioCallType::WriteDigital,
                10,
                std::nullopt,
                true);
            ok &= expectCall(
                "forward pulse low " + std::to_string(i),
                forward_calls[static_cast<std::size_t>(3 + i * 2)],
                gte::GpioCallType::WriteDigital,
                10,
                std::nullopt,
                false);
        }
    }
    ok &= expectTrue("Forward move updates current step", axis.currentStep() == 3);
    ok &= expectNear("Forward move updates angle", axis.currentAngleDeg(), 0.75, 1.0e-12);

    gpio.clearCalls();
    axis.moveSteps(-2);
    const auto& backward_calls = gpio.calls();
    ok &= expectTrue("Backward move stays enabled, sets direction, and pulses", backward_calls.size() == 5);
    if (backward_calls.size() == 5) {
        ok &= expectCall("backward direction", backward_calls[0], gte::GpioCallType::WriteDigital, 11, std::nullopt, false);
        ok &= expectCall("backward pulse high 0", backward_calls[1], gte::GpioCallType::WriteDigital, 10, std::nullopt, true);
        ok &= expectCall("backward pulse low 0", backward_calls[2], gte::GpioCallType::WriteDigital, 10, std::nullopt, false);
        ok &= expectCall("backward pulse high 1", backward_calls[3], gte::GpioCallType::WriteDigital, 10, std::nullopt, true);
        ok &= expectCall("backward pulse low 1", backward_calls[4], gte::GpioCallType::WriteDigital, 10, std::nullopt, false);
    }
    ok &= expectTrue("Backward move updates current step", axis.currentStep() == 1);

    gpio.clearCalls();
    axis.moveSteps(0);
    ok &= expectTrue("Zero-step move produces no GPIO calls", gpio.calls().empty());

    axis.disable();
    ok &= expectTrue("Axis disables", !axis.enabled());

    gte::FakeGpio mount_gpio;
    gte::StepperAxis altitude_axis(mount_gpio, {
        .step_pin = 20,
        .direction_pin = 21,
        .enable_pin = 22,
        .steps_per_degree = 10.0,
    });
    gte::StepperAxis azimuth_axis(mount_gpio, {
        .step_pin = 30,
        .direction_pin = 31,
        .enable_pin = 32,
        .steps_per_degree = 5.0,
    });
    gte::StepperMount mount(altitude_axis, azimuth_axis);

    mount.slewTo(1.2, 2.0);
    ok &= expectTrue("Stepper mount altitude step target", altitude_axis.currentStep() == 12);
    ok &= expectTrue("Stepper mount azimuth step target", azimuth_axis.currentStep() == 10);
    ok &= expectNear("Stepper mount current altitude", mount.currentPosition().alt_deg, 1.2, 1.0e-12);
    ok &= expectNear("Stepper mount current azimuth", mount.currentPosition().az_deg, 2.0, 1.0e-12);

    mount.slewTo(0.7, 1.0);
    ok &= expectTrue("Stepper mount altitude can move backward", altitude_axis.currentStep() == 7);
    ok &= expectTrue("Stepper mount azimuth can move backward", azimuth_axis.currentStep() == 5);
    ok &= expectNear("Stepper mount updated altitude", mount.currentPosition().alt_deg, 0.7, 1.0e-12);
    ok &= expectNear("Stepper mount updated azimuth", mount.currentPosition().az_deg, 1.0, 1.0e-12);

    if (ok) {
        std::cout << "All stepper tests passed\n";
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
