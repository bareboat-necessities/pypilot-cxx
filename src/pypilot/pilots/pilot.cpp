#include "pypilot/pilots/pilot.hpp"

namespace pypilot {

Pilot::Pilot(PypilotState& state, std::string name) : state_(state), name_(std::move(name)) {}

void Pilot::compute_heading()
{
    // TODO: translate base pilot heading computation hooks from pilots/pilot.py.
}

void Pilot::process()
{
    // TODO: translate base pilot process interface and gain handling.
    state_.servo.command = state_.ap.command;
}

} // namespace pypilot
