#include "pypilot/pilots/pilot.hpp"

namespace pypilot {

GpsPilot::GpsPilot(PypilotState& state) : Pilot(state, "gps") {}

void GpsPilot::compute_heading()
{
    // TODO: translate gps.py route/APB/GPS heading behavior.
    if (state_.navigation.apb.track_deg.valid) state_.ap.heading_command = state_.navigation.apb.track_deg.value;
}

} // namespace pypilot
