#include "pypilot/pypilot.hpp"

namespace pypilot {

Rudder::Rudder(PypilotState& state) : state_(state) {}

void Rudder::poll(uint64_t now_us)
{
    (void)now_us;
    // TODO: translate rudder.py calibration, range/offset handling and timeout behavior.
    (void)state_;
}

} // namespace pypilot
