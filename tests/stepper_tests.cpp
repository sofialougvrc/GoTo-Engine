#include "gte/stepper_axis.hpp"
#include "gte/stepper_mount.hpp"
#include "gte/time_utils.hpp"
#include "gte/tracking_loop.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
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

std::chrono::system_clock::time_point makeUtc(
    int year,
    unsigned month,
    unsigned day,
    int hour,
    int minute,
    int second) {
    using namespace std::chrono;

    const auto date = sys_days{std::chrono::year{year} / month / day};
    const auto timestamp = date + hours{hour} + minutes{minute} + seconds{second};
    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(timestamp);
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

    const gte::MountStepCalibration direct_mount_calibration{
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

    gte::FakeGpio mount_gpio;
    gte::StepperMount mount(mount_gpio, {
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
        .calibration = direct_mount_calibration,
    });

    mount.initialize();
    mount_gpio.clearCalls();
    mount.slewTo(1.2, 2.0);
    const auto& mount_forward_calls = mount_gpio.calls();
    ok &= expectTrue("Stepper mount direct slew emits expected call count", mount_forward_calls.size() == 48);
    ok &= expectTrue("Stepper mount altitude step target", mount.altitudeAxis().currentStep() == 12);
    ok &= expectTrue("Stepper mount azimuth step target", mount.azimuthAxis().currentStep() == 10);
    ok &= expectNear("Stepper mount current altitude", mount.currentPosition().alt_deg, 1.2, 1.0e-12);
    ok &= expectNear("Stepper mount current azimuth", mount.currentPosition().az_deg, 2.0, 1.0e-12);
    ok &= expectTrue("Stepper mount enables altitude axis", countWrites(mount_forward_calls, 22, false) == 1);
    ok &= expectTrue("Stepper mount enables azimuth axis", countWrites(mount_forward_calls, 32, false) == 1);
    ok &= expectTrue("Stepper mount altitude direction positive", countWrites(mount_forward_calls, 21, true) == 1);
    ok &= expectTrue("Stepper mount azimuth direction positive", countWrites(mount_forward_calls, 31, true) == 1);
    ok &= expectTrue("Stepper mount altitude high pulses", countWrites(mount_forward_calls, 20, true) == 12);
    ok &= expectTrue("Stepper mount altitude low pulses", countWrites(mount_forward_calls, 20, false) == 12);
    ok &= expectTrue("Stepper mount azimuth high pulses", countWrites(mount_forward_calls, 30, true) == 10);
    ok &= expectTrue("Stepper mount azimuth low pulses", countWrites(mount_forward_calls, 30, false) == 10);

    mount_gpio.clearCalls();
    mount.slewTo(0.7, 1.0);
    const auto& mount_backward_calls = mount_gpio.calls();
    ok &= expectTrue("Stepper mount backward slew emits expected call count", mount_backward_calls.size() == 22);
    ok &= expectTrue("Stepper mount altitude can move backward", mount.altitudeAxis().currentStep() == 7);
    ok &= expectTrue("Stepper mount azimuth can move backward", mount.azimuthAxis().currentStep() == 5);
    ok &= expectNear("Stepper mount updated altitude", mount.currentPosition().alt_deg, 0.7, 1.0e-12);
    ok &= expectNear("Stepper mount updated azimuth", mount.currentPosition().az_deg, 1.0, 1.0e-12);
    ok &= expectTrue("Stepper mount does not re-enable altitude axis", countWrites(mount_backward_calls, 22, false) == 0);
    ok &= expectTrue("Stepper mount does not re-enable azimuth axis", countWrites(mount_backward_calls, 32, false) == 0);
    ok &= expectTrue("Stepper mount altitude direction negative", countWrites(mount_backward_calls, 21, false) == 1);
    ok &= expectTrue("Stepper mount azimuth direction negative", countWrites(mount_backward_calls, 31, false) == 1);
    ok &= expectTrue("Stepper mount backward altitude high pulses", countWrites(mount_backward_calls, 20, true) == 5);
    ok &= expectTrue("Stepper mount backward altitude low pulses", countWrites(mount_backward_calls, 20, false) == 5);
    ok &= expectTrue("Stepper mount backward azimuth high pulses", countWrites(mount_backward_calls, 30, true) == 5);
    ok &= expectTrue("Stepper mount backward azimuth low pulses", countWrites(mount_backward_calls, 30, false) == 5);

    const gte::MountStepCalibration tracking_calibration{
        .altitude = {
            .motor_steps_per_revolution = 200,
            .gearbox_ratio = 1.0,
            .microsteps_per_full_step = 1,
            .external_gear_reduction = 1.8,
        },
        .azimuth = {
            .motor_steps_per_revolution = 200,
            .gearbox_ratio = 1.0,
            .microsteps_per_full_step = 1,
            .external_gear_reduction = 1.8,
        },
    };

    gte::FakeGpio tracking_gpio;
    gte::StepperMount tracking_mount(tracking_gpio, {
        .altitude_pins = {
            .step_pin = 40,
            .direction_pin = 41,
            .enable_pin = 42,
        },
        .azimuth_pins = {
            .step_pin = 50,
            .direction_pin = 51,
            .enable_pin = 52,
        },
        .calibration = tracking_calibration,
    });
    tracking_mount.initialize();
    tracking_gpio.clearCalls();

    const auto tracking_time = makeUtc(2026, 1, 1, 0, 0, 0);
    const double lst = gte::greenwichMeanSiderealTime(tracking_time);
    gte::TrackingLoop tracking_loop(tracking_mount, 0.0, 0.0);
    tracking_loop.trackObject({.ra_deg = lst, .dec_deg = 0.0});
    tracking_loop.tickAt(tracking_time);

    const auto& tracking_calls = tracking_gpio.calls();
    ok &= expectNear("Stepper mount tracking altitude", tracking_mount.currentPosition().alt_deg, 90.0, 1.0e-12);
    ok &= expectNear("Stepper mount tracking azimuth", tracking_mount.currentPosition().az_deg, 0.0, 1.0e-12);
    ok &= expectTrue("Tracking loop drives StepperMount altitude direction positive", countWrites(tracking_calls, 41, true) == 1);
    ok &= expectTrue("Tracking loop emits ninety altitude high pulses", countWrites(tracking_calls, 40, true) == 90);
    ok &= expectTrue("Tracking loop emits ninety altitude low pulses", countWrites(tracking_calls, 40, false) == 90);
    ok &= expectTrue("Tracking loop does not move azimuth axis at zenith", countWrites(tracking_calls, 50, true) == 0);
    ok &= expectTrue("Tracking StepperMount stores altitude steps", tracking_mount.altitudeAxis().currentStep() == 90);
    ok &= expectTrue("Tracking StepperMount stores azimuth steps", tracking_mount.azimuthAxis().currentStep() == 0);

    if (ok) {
        std::cout << "All stepper tests passed\n";
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
