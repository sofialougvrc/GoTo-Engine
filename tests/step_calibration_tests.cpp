#include "gte/step_calibration.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
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

template <typename Callable>
bool expectThrows(const std::string& name, Callable callable) {
    try {
        callable();
    } catch (const std::invalid_argument&) {
        return true;
    } catch (...) {
        std::cerr << name << " failed: threw unexpected exception type\n";
        return false;
    }

    std::cerr << name << " failed: did not throw\n";
    return false;
}

} // namespace

int main() {
    bool ok = true;

    const gte::AxisStepCalibration direct_pg19{
        .motor_steps_per_revolution = 200,
        .gearbox_ratio = 19.19,
        .microsteps_per_full_step = 16,
        .external_gear_reduction = 1.0,
    };

    // 200 * 16 * 19.19 * 1.0 = 61408 output microsteps/rev.
    // 61408 / 360 = 170.577777... microsteps/degree.
    ok &= expectNear(
        "PG19 direct steps per revolution",
        direct_pg19.stepsPerOutputRevolution(),
        61408.0,
        1.0e-9);
    ok &= expectNear(
        "PG19 direct steps per degree",
        direct_pg19.stepsPerDegree(),
        170.57777777777778,
        1.0e-12);
    ok &= expectTrue(
        "PG19 direct ten degrees rounds to steps",
        direct_pg19.degreesToSteps(10.0) == 1706);
    ok &= expectTrue(
        "PG19 direct negative delta rounds to steps",
        direct_pg19.degreesToSteps(-2.5) == -426);

    const gte::AxisStepCalibration two_to_one_pg19{
        .motor_steps_per_revolution = 200,
        .gearbox_ratio = 19.19,
        .microsteps_per_full_step = 16,
        .external_gear_reduction = 2.0,
    };

    // 200 * 16 * 19.19 * 2.0 = 122816 output microsteps/rev.
    // 122816 / 360 = 341.155555... microsteps/degree.
    ok &= expectNear(
        "PG19 2:1 steps per revolution",
        two_to_one_pg19.stepsPerOutputRevolution(),
        122816.0,
        1.0e-9);
    ok &= expectNear(
        "PG19 2:1 steps per degree",
        two_to_one_pg19.stepsPerDegree(),
        341.15555555555557,
        1.0e-12);
    ok &= expectTrue(
        "PG19 2:1 quarter degree rounds to steps",
        two_to_one_pg19.degreesToSteps(0.25) == 85);

    const gte::AxisStepCalibration three_to_one_pg19{
        .motor_steps_per_revolution = 200,
        .gearbox_ratio = 19.19,
        .microsteps_per_full_step = 8,
        .external_gear_reduction = 3.0,
    };

    // 200 * 8 * 19.19 * 3.0 = 92112 output microsteps/rev.
    // 92112 / 360 = 255.866666... microsteps/degree.
    ok &= expectNear(
        "PG19 3:1 at 8 microsteps steps per revolution",
        three_to_one_pg19.stepsPerOutputRevolution(),
        92112.0,
        1.0e-9);
    ok &= expectNear(
        "PG19 3:1 at 8 microsteps steps per degree",
        three_to_one_pg19.stepsPerDegree(),
        255.86666666666667,
        1.0e-12);

    const gte::MountStepCalibration mount_calibration{
        .altitude = two_to_one_pg19,
        .azimuth = three_to_one_pg19,
    };
    const gte::AxisStepDelta delta = gte::horizontalDeltaToSteps(
        {.alt_deg = 1.5, .az_deg = -2.25},
        mount_calibration);

    // Alt: 1.5 * 341.155555... = 511.7333... -> 512.
    // Az: -2.25 * 255.866666... = -575.7 -> -576.
    ok &= expectTrue("Horizontal altitude delta to steps", delta.altitude_steps == 512);
    ok &= expectTrue("Horizontal azimuth delta to steps", delta.azimuth_steps == -576);

    ok &= expectThrows("Rejects zero motor steps", [] {
        gte::AxisStepCalibration invalid = {
            .motor_steps_per_revolution = 0,
            .gearbox_ratio = 19.19,
            .microsteps_per_full_step = 16,
            .external_gear_reduction = 1.0,
        };
        (void)invalid.stepsPerDegree();
    });
    ok &= expectThrows("Rejects zero external reduction", [] {
        gte::AxisStepCalibration invalid = {
            .motor_steps_per_revolution = 200,
            .gearbox_ratio = 19.19,
            .microsteps_per_full_step = 16,
            .external_gear_reduction = 0.0,
        };
        (void)invalid.stepsPerDegree();
    });

    if (ok) {
        std::cout << "All step calibration tests passed\n";
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
