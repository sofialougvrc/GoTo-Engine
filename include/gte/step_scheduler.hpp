#pragma once

#include "gte/gpio_interface.hpp"
#include "gte/stepper_axis.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

namespace gte {

struct StepSchedulerRates {
    double altitude_steps_per_second = 0.0;
    double azimuth_steps_per_second = 0.0;
};

struct StepSchedulerTargets {
    std::int64_t altitude_step = 0;
    std::int64_t azimuth_step = 0;
};

struct StepSchedulerConfig {
    StepperAxisConfig altitude_axis;
    StepperAxisConfig azimuth_axis;
    std::chrono::microseconds pulse_width{50};
};

class StepScheduler {
public:
    StepScheduler(GpioInterface& gpio, StepSchedulerConfig config);
    ~StepScheduler();

    StepScheduler(const StepScheduler&) = delete;
    StepScheduler& operator=(const StepScheduler&) = delete;

    void initialize();
    void start();
    void stop();

    void enable();
    void disable();

    void setTargetRates(StepSchedulerRates rates);
    void moveToTargetsAtRates(StepSchedulerTargets targets, StepSchedulerRates rates);
    void moveToStepTargetsBlocking(StepSchedulerTargets targets);

    StepSchedulerRates targetRates() const;
    StepSchedulerTargets currentSteps() const;
    bool running() const;

private:
    struct AxisState {
        StepperAxisConfig config;
        double rate_steps_per_second = 0.0;
        std::optional<std::int64_t> target_step;
        std::int64_t current_step = 0;
        bool enabled = false;
        bool initialized = false;
        bool direction_positive = true;
        bool direction_written = false;
        std::chrono::steady_clock::time_point next_pulse{};
    };

    void workerLoop();
    void initializeAxis(AxisState& axis);
    void enableAxis(AxisState& axis);
    void disableAxis(AxisState& axis);
    void pulseAxis(AxisState& axis);
    void updateDirection(AxisState& axis, bool positive);
    bool axisHasWork(const AxisState& axis) const;
    void stopAxisAtTargetIfReached(AxisState& axis);
    void resetNextPulseLocked(AxisState& axis, std::chrono::steady_clock::time_point now);
    static bool enablePinValue(const AxisState& axis, bool enabled);
    static bool directionPinValue(const AxisState& axis, bool positive);

    GpioInterface& gpio_;
    StepSchedulerConfig config_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    AxisState altitude_;
    AxisState azimuth_;
    bool running_ = false;
    bool stop_requested_ = false;
    std::thread worker_;
};

} // namespace gte
