#include "gte/step_scheduler.hpp"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

bool expectTrue(const std::string& name, bool condition) {
    if (condition) {
        return true;
    }

    std::cerr << name << " failed\n";
    return false;
}

std::vector<std::chrono::steady_clock::time_point> highPulseTimes(
    const std::vector<gte::GpioCall>& calls,
    int pin) {
    std::vector<std::chrono::steady_clock::time_point> times;
    for (const auto& call : calls) {
        if (call.type == gte::GpioCallType::WriteDigital &&
            call.pin == pin &&
            call.value == true) {
            times.push_back(call.timestamp);
        }
    }
    return times;
}

std::size_t countWrites(const std::vector<gte::GpioCall>& calls, int pin, bool value) {
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

    gte::FakeGpio gpio;
    gte::StepScheduler scheduler(gpio, {
        .altitude_axis = {
            .step_pin = 10,
            .direction_pin = 11,
            .enable_pin = 12,
            .steps_per_degree = 1.0,
        },
        .azimuth_axis = {
            .step_pin = 20,
            .direction_pin = 21,
            .enable_pin = 22,
            .steps_per_degree = 1.0,
        },
        .pulse_width = std::chrono::microseconds{0},
    });

    scheduler.initialize();
    scheduler.start();
    gpio.clearCalls();
    scheduler.setTargetRates({
        .altitude_steps_per_second = 40.0,
        .azimuth_steps_per_second = -20.0,
    });

    std::this_thread::sleep_for(std::chrono::milliseconds{160});
    scheduler.stop();

    const auto calls = gpio.calls();
    const auto altitude_pulses = highPulseTimes(calls, 10);
    const auto azimuth_pulses = highPulseTimes(calls, 20);

    ok &= expectTrue("Scheduler emits altitude pulse count near rate", altitude_pulses.size() >= 5 && altitude_pulses.size() <= 7);
    ok &= expectTrue("Scheduler emits azimuth pulse count near rate", azimuth_pulses.size() >= 2 && azimuth_pulses.size() <= 4);
    ok &= expectTrue("Scheduler writes positive altitude direction", countWrites(calls, 11, true) == 1);
    ok &= expectTrue("Scheduler writes negative azimuth direction", countWrites(calls, 21, false) == 1);
    ok &= expectTrue("Scheduler altitude high/low pulse counts match", countWrites(calls, 10, true) == countWrites(calls, 10, false));
    ok &= expectTrue("Scheduler azimuth high/low pulse counts match", countWrites(calls, 20, true) == countWrites(calls, 20, false));

    if (altitude_pulses.size() >= 3) {
        for (std::size_t i = 1; i < altitude_pulses.size(); ++i) {
            const auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                altitude_pulses[i] - altitude_pulses[i - 1]);
            ok &= expectTrue(
                "Scheduler altitude pulse interval within tolerance",
                interval >= std::chrono::milliseconds{15} &&
                interval <= std::chrono::milliseconds{40});
        }
    }

    const gte::StepSchedulerTargets steps = scheduler.currentSteps();
    ok &= expectTrue("Scheduler altitude current step matches pulse count", steps.altitude_step == static_cast<std::int64_t>(altitude_pulses.size()));
    ok &= expectTrue("Scheduler azimuth current step matches pulse count", steps.azimuth_step == -static_cast<std::int64_t>(azimuth_pulses.size()));

    gpio.clearCalls();
    scheduler.start();
    scheduler.moveToTargetsAtRates(
        {.altitude_step = steps.altitude_step + 4, .azimuth_step = steps.azimuth_step + 3},
        {.altitude_steps_per_second = 80.0, .azimuth_steps_per_second = 80.0});
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    scheduler.stop();

    const auto target_calls = gpio.calls();
    const gte::StepSchedulerTargets final_steps = scheduler.currentSteps();
    ok &= expectTrue("Scheduler target-limited altitude stops exactly", final_steps.altitude_step == steps.altitude_step + 4);
    ok &= expectTrue("Scheduler target-limited azimuth stops exactly", final_steps.azimuth_step == steps.azimuth_step + 3);
    ok &= expectTrue("Scheduler target-limited altitude pulses", countWrites(target_calls, 10, true) == 4);
    ok &= expectTrue("Scheduler target-limited azimuth pulses", countWrites(target_calls, 20, true) == 3);

    if (ok) {
        std::cout << "All step scheduler tests passed\n";
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
