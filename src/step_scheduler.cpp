#include "gte/step_scheduler.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace gte {
namespace {

void validateAxisConfig(const StepperAxisConfig& config) {
    if (config.steps_per_degree <= 0.0) {
        throw std::invalid_argument("StepScheduler steps_per_degree must be positive");
    }
    if (config.step_pin == config.direction_pin ||
        config.step_pin == config.enable_pin ||
        config.direction_pin == config.enable_pin) {
        throw std::invalid_argument("StepScheduler axis pins must be distinct");
    }
}

std::chrono::steady_clock::duration periodForRate(double rate_steps_per_second) {
    const double seconds = 1.0 / std::abs(rate_steps_per_second);
    return std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(seconds));
}

} // namespace

StepScheduler::StepScheduler(GpioInterface& gpio, StepSchedulerConfig config)
    : gpio_(gpio),
      config_(config),
      altitude_{.config = config_.altitude_axis},
      azimuth_{.config = config_.azimuth_axis} {
    validateAxisConfig(config_.altitude_axis);
    validateAxisConfig(config_.azimuth_axis);
}

StepScheduler::~StepScheduler() {
    stop();
}

void StepScheduler::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    initializeAxis(altitude_);
    initializeAxis(azimuth_);
}

void StepScheduler::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return;
    }

    stop_requested_ = false;
    running_ = true;
    worker_ = std::thread(&StepScheduler::workerLoop, this);
}

void StepScheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_) {
            return;
        }
        stop_requested_ = true;
        altitude_.rate_steps_per_second = 0.0;
        azimuth_.rate_steps_per_second = 0.0;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
    stop_requested_ = false;
}

void StepScheduler::enable() {
    std::lock_guard<std::mutex> lock(mutex_);
    initializeAxis(altitude_);
    initializeAxis(azimuth_);
    enableAxis(altitude_);
    enableAxis(azimuth_);
}

void StepScheduler::disable() {
    std::lock_guard<std::mutex> lock(mutex_);
    initializeAxis(altitude_);
    initializeAxis(azimuth_);
    altitude_.rate_steps_per_second = 0.0;
    azimuth_.rate_steps_per_second = 0.0;
    disableAxis(altitude_);
    disableAxis(azimuth_);
    cv_.notify_all();
}

void StepScheduler::setTargetRates(StepSchedulerRates rates) {
    std::lock_guard<std::mutex> lock(mutex_);
    initializeAxis(altitude_);
    initializeAxis(azimuth_);

    altitude_.target_step.reset();
    azimuth_.target_step.reset();
    altitude_.rate_steps_per_second = rates.altitude_steps_per_second;
    azimuth_.rate_steps_per_second = rates.azimuth_steps_per_second;
    altitude_.direction_written = false;
    azimuth_.direction_written = false;

    const auto now = std::chrono::steady_clock::now();
    resetNextPulseLocked(altitude_, now);
    resetNextPulseLocked(azimuth_, now);
    cv_.notify_all();
}

void StepScheduler::moveToTargetsAtRates(
    StepSchedulerTargets targets,
    StepSchedulerRates rates) {
    std::lock_guard<std::mutex> lock(mutex_);
    initializeAxis(altitude_);
    initializeAxis(azimuth_);

    altitude_.target_step = targets.altitude_step;
    azimuth_.target_step = targets.azimuth_step;
    altitude_.rate_steps_per_second =
        targets.altitude_step == altitude_.current_step ? 0.0 : rates.altitude_steps_per_second;
    azimuth_.rate_steps_per_second =
        targets.azimuth_step == azimuth_.current_step ? 0.0 : rates.azimuth_steps_per_second;
    altitude_.direction_written = false;
    azimuth_.direction_written = false;

    stopAxisAtTargetIfReached(altitude_);
    stopAxisAtTargetIfReached(azimuth_);

    const auto now = std::chrono::steady_clock::now();
    resetNextPulseLocked(altitude_, now);
    resetNextPulseLocked(azimuth_, now);
    cv_.notify_all();
}

void StepScheduler::moveToStepTargetsBlocking(StepSchedulerTargets targets) {
    std::lock_guard<std::mutex> lock(mutex_);
    initializeAxis(altitude_);
    initializeAxis(azimuth_);
    altitude_.rate_steps_per_second = 0.0;
    azimuth_.rate_steps_per_second = 0.0;
    altitude_.target_step.reset();
    azimuth_.target_step.reset();
    altitude_.direction_written = false;
    azimuth_.direction_written = false;

    while (altitude_.current_step != targets.altitude_step ||
           azimuth_.current_step != targets.azimuth_step) {
        if (altitude_.current_step != targets.altitude_step) {
            updateDirection(altitude_, targets.altitude_step > altitude_.current_step);
            enableAxis(altitude_);
            pulseAxis(altitude_);
        }
        if (azimuth_.current_step != targets.azimuth_step) {
            updateDirection(azimuth_, targets.azimuth_step > azimuth_.current_step);
            enableAxis(azimuth_);
            pulseAxis(azimuth_);
        }
    }
}

StepSchedulerRates StepScheduler::targetRates() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {
        altitude_.rate_steps_per_second,
        azimuth_.rate_steps_per_second,
    };
}

StepSchedulerTargets StepScheduler::currentSteps() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {
        altitude_.current_step,
        azimuth_.current_step,
    };
}

bool StepScheduler::running() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void StepScheduler::workerLoop() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (!stop_requested_) {
        if (!axisHasWork(altitude_) && !axisHasWork(azimuth_)) {
            cv_.wait(lock, [this] {
                return stop_requested_ || axisHasWork(altitude_) || axisHasWork(azimuth_);
            });
            continue;
        }

        const auto now = std::chrono::steady_clock::now();
        auto next_wake = now + std::chrono::hours(24);
        bool emitted = false;

        for (AxisState* axis : {&altitude_, &azimuth_}) {
            stopAxisAtTargetIfReached(*axis);
            if (!axisHasWork(*axis)) {
                continue;
            }

            if (axis->next_pulse <= now) {
                updateDirection(*axis, axis->rate_steps_per_second > 0.0);
                enableAxis(*axis);
                pulseAxis(*axis);
                axis->next_pulse += periodForRate(axis->rate_steps_per_second);
                if (axis->next_pulse <= now) {
                    axis->next_pulse = now + periodForRate(axis->rate_steps_per_second);
                }
                stopAxisAtTargetIfReached(*axis);
                emitted = true;
            }

            if (axisHasWork(*axis)) {
                next_wake = std::min(next_wake, axis->next_pulse);
            }
        }

        if (!emitted) {
            cv_.wait_until(lock, next_wake, [this] { return stop_requested_; });
        }
    }
}

void StepScheduler::initializeAxis(AxisState& axis) {
    if (axis.initialized) {
        return;
    }

    gpio_.setMode(axis.config.step_pin, GpioPinMode::Output);
    gpio_.setMode(axis.config.direction_pin, GpioPinMode::Output);
    gpio_.setMode(axis.config.enable_pin, GpioPinMode::Output);
    gpio_.writeDigitalPin(axis.config.step_pin, false);
    disableAxis(axis);
    axis.initialized = true;
}

void StepScheduler::enableAxis(AxisState& axis) {
    if (axis.enabled) {
        return;
    }
    gpio_.writeDigitalPin(axis.config.enable_pin, enablePinValue(axis, true));
    axis.enabled = true;
}

void StepScheduler::disableAxis(AxisState& axis) {
    gpio_.writeDigitalPin(axis.config.enable_pin, enablePinValue(axis, false));
    axis.enabled = false;
}

void StepScheduler::pulseAxis(AxisState& axis) {
    gpio_.writeDigitalPin(axis.config.step_pin, true);
    if (config_.pulse_width.count() > 0) {
        std::this_thread::sleep_for(config_.pulse_width);
    }
    gpio_.writeDigitalPin(axis.config.step_pin, false);
    axis.current_step += axis.direction_positive ? 1 : -1;
}

void StepScheduler::updateDirection(AxisState& axis, bool positive) {
    if (axis.direction_written && axis.direction_positive == positive) {
        return;
    }

    axis.direction_positive = positive;
    axis.direction_written = true;
    gpio_.writeDigitalPin(axis.config.direction_pin, directionPinValue(axis, positive));
}

bool StepScheduler::axisHasWork(const AxisState& axis) const {
    return axis.rate_steps_per_second != 0.0;
}

void StepScheduler::stopAxisAtTargetIfReached(AxisState& axis) {
    if (!axis.target_step.has_value()) {
        return;
    }

    if ((axis.rate_steps_per_second > 0.0 && axis.current_step >= *axis.target_step) ||
        (axis.rate_steps_per_second < 0.0 && axis.current_step <= *axis.target_step) ||
        axis.current_step == *axis.target_step) {
        axis.current_step = *axis.target_step;
        axis.rate_steps_per_second = 0.0;
        axis.target_step.reset();
    }
}

void StepScheduler::resetNextPulseLocked(
    AxisState& axis,
    std::chrono::steady_clock::time_point now) {
    if (axis.rate_steps_per_second == 0.0) {
        return;
    }
    axis.next_pulse = now + periodForRate(axis.rate_steps_per_second);
}

bool StepScheduler::enablePinValue(const AxisState& axis, bool enabled) {
    return axis.config.enable_active_low ? !enabled : enabled;
}

bool StepScheduler::directionPinValue(const AxisState& axis, bool positive) {
    return positive == axis.config.positive_direction_high;
}

} // namespace gte
