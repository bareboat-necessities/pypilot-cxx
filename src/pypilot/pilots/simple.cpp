#include "pypilot/pilots/pilot.hpp"

namespace pypilot {

SimplePilot::SimplePilot(PypilotState& state) : Pilot(state, "simple") {}

void SimplePilot::process()
{
    // TODO: translate simple.py behavior.
    Pilot::process();
}

} // namespace pypilot
