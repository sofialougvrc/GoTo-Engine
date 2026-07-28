#include "gte/tracking_loop.hpp"

#include "gte/time_utils.hpp"

#include <stdexcept>

namespace gte {

TrackingLoop::TrackingLoop(
    IMountDriver& mount,
    double observer_lat_deg,
    double observer_lon_deg)
    : mount_(mount),
      observer_lat_(observer_lat_deg),
      observer_lon_(observer_lon_deg) {}

void TrackingLoop::trackObject(EquatorialCoord target) {
    target_ = target;
    has_target_ = true;
}

void TrackingLoop::tick() {
    tickAt(std::chrono::system_clock::now());
}

void TrackingLoop::tickAt(std::chrono::system_clock::time_point utc) {
    if (!has_target_) {
        throw std::logic_error("TrackingLoop::trackObject must be called before tick");
    }

    const double lst = localSiderealTime(utc, observer_lon_);
    const HorizontalCoord horizontal =
        equatorialToHorizontal(target_, observer_lat_, lst);

    mount_.track(horizontal.alt_deg, horizontal.az_deg);
}

} // namespace gte
