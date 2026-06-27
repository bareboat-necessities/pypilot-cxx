#include "pypilot/pilots/pilot.hpp"

namespace pypilot {

WindPilot::WindPilot(PypilotState& state) : Pilot(state, "wind") {}

void WindPilot::compute_heading()
{
    // TODO: translate wind.py behavior.
    if (state_.wind.apparent_direction.valid) state_.ap.heading_command = state_.wind.apparent_direction.value;
}

} // namespace pypilot
