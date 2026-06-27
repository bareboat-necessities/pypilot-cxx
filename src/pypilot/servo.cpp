#include "pypilot/pypilot.hpp"

#include <algorithm>

namespace pypilot {

Servo::Servo(EventLoop& loop, PypilotState& state) : loop_(loop), state_(state) {}

void Servo::command(double value)
{
    value = std::clamp(value, -1.0, 1.0);
    state_.servo.command = static_cast<Real>(value);
    // TODO: translate servo.py limits, timeout, fault and calibration logic.
}

void Servo::set_autopilot_enabled(bool enabled)
{
    state_.servo.engaged = enabled;
}

void Servo::poll()
{
    (void)loop_;
    // TODO: translate servo serial protocol, current/voltage/fault feedback,
    // and Arduino/Linux backend separation.
}

} // namespace pypilot
