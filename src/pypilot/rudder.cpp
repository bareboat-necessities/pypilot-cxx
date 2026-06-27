#include "pypilot/pypilot.hpp"

#include <cmath>

namespace pypilot {
namespace {
Real round2(double value)
{
    return static_cast<Real>(std::round(value * 100.0) / 100.0);
}
}

Rudder::Rudder(PypilotState& state) : state_(state)
{
    update_minmax();
}

void Rudder::update_minmax()
{
    auto& r = state_.rudder;
    const Real scale = std::fabs(r.scale) < static_cast<Real>(0.01) ? static_cast<Real>(100.0) : r.scale;
    r.min_raw = static_cast<Real>((-r.range - r.offset) / scale);
    r.max_raw = static_cast<Real>(( r.range - r.offset) / scale);
    if (r.min_raw > r.max_raw) std::swap(r.min_raw, r.max_raw);
    r.min_raw = std::clamp(r.min_raw, static_cast<Real>(-0.5), static_cast<Real>(0.5));
    r.max_raw = std::clamp(r.max_raw, static_cast<Real>(-0.5), static_cast<Real>(0.5));
}

bool Rudder::invalid() const
{
    return !state_.rudder.angle.valid;
}

bool Rudder::update(double raw, std::string_view device, uint64_t now_us)
{
    auto& r = state_.rudder;
    if (device != r.last_device) {
        r.last_device = std::string(device);
        reset();
        return false;
    }
    if (std::isnan(raw)) {
        reset();
        return false;
    }

    r.raw = static_cast<Real>(raw);
    r.raw_valid = true;
    r.raw_timestamp_us = now_us;
    update_minmax();

    const double angle = r.scale * raw + r.offset + r.nonlinearity * (r.min_raw - raw) * (r.max_raw - raw);
    const Real rounded = round2(angle);

    if (r.last_angle_time_us != 0 && now_us > r.last_angle_time_us) {
        double dt = static_cast<double>(now_us - r.last_angle_time_us) * 1.0e-6;
        if (dt > 1.0) dt = 1.0;
        if (dt > 0.0) {
            const Real speed = static_cast<Real>((rounded - r.last_angle) / dt);
            r.speed.value = static_cast<Real>(0.9 * r.speed.value + 0.1 * speed);
            r.speed.timestamp_us = now_us;
            r.speed.source = SensorSource::servo;
            r.speed.valid = true;
        }
    }

    r.angle.value = rounded;
    r.angle.timestamp_us = now_us;
    r.angle.source = SensorSource::servo;
    r.angle.valid = true;
    r.last_angle = rounded;
    r.last_angle_time_us = now_us;
    return true;
}

void Rudder::reset()
{
    state_.rudder.angle.valid = false;
    state_.rudder.speed.valid = false;
    state_.rudder.raw_valid = false;
}

void Rudder::poll(uint64_t now_us)
{
    auto& r = state_.rudder;
    update_minmax();

    if (r.raw_valid && r.raw_timestamp_us != r.last_angle_time_us) {
        update(r.raw, r.last_device.empty() ? std::string_view("servo") : std::string_view(r.last_device), now_us);
    }

    if (r.calibration_state == "reset") {
        r.nonlinearity = 0;
        r.scale = 100;
        r.offset = 0;
        r.calibration_state = "idle";
        update_minmax();
    }
    // TODO: translate centered/starboard range/port range calibration point fitting from rudder.py.
    // TODO: translate auto gain state machine once servo command timing is complete.
}

} // namespace pypilot
