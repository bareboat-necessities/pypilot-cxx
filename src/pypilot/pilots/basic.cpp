#include "pypilot/pilots/pilot.hpp"

#include <algorithm>

namespace pypilot {

BasicPilot::BasicPilot(PypilotState& state) : Pilot(state, "basic") {}

void BasicPilot::process()
{
    // Initial placeholder: proportional output from heading error.
    // TODO: translate exact basic.py PID/gain terms and servo output rules.
    state_.ap.command = static_cast<Real>(std::clamp(state_.ap.heading_error / 30.0f, -1.0f, 1.0f));
    state_.servo.command = state_.ap.command;
}

} // namespace pypilot
