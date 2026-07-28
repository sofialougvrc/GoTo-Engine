#pragma once

#include "gte/coord_transform.hpp"

namespace gte {

class IMountDriver {
public:
    virtual void slewTo(double alt_deg, double az_deg) = 0;
    virtual void track(double alt_deg, double az_deg) { slewTo(alt_deg, az_deg); }
    virtual HorizontalCoord currentPosition() const = 0;
    virtual ~IMountDriver() = default;
};

class VirtualMount : public IMountDriver {
public:
    void slewTo(double alt_deg, double az_deg) override;
    HorizontalCoord currentPosition() const override;

private:
    HorizontalCoord current_{0.0, 0.0};
};

} // namespace gte
