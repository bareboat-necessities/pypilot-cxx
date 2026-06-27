#include "pypilot/pypilot.hpp"

namespace pypilot {

Tacking::Tacking(PypilotState& state) : state_(state) {}

bool Tacking::process()
{
    // TODO: translate tacking.py state machine and heading-command shaping.
    return state_.ap.tack.state;
}

void Tacking::poll(uint64_t now_us)
{
    (void)now_us;
    // TODO: translate tack timeout and completion logic.
}

} // namespace pypilot
