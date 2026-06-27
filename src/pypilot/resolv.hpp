#pragma once

namespace pypilot {

inline double resolv(double angle)
{
    while (angle < -180.0) angle += 360.0;
    while (angle >= 180.0) angle -= 360.0;
    return angle;
}

inline double heading_error(double target_deg, double current_deg)
{
    return resolv(target_deg - current_deg);
}

inline double compass_normalize(double angle)
{
    while (angle < 0.0) angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;
    return angle;
}

} // namespace pypilot
