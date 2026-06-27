#include "pypilot/pypilot.hpp"

namespace pypilot {

Sensors::Sensors(PypilotState& state) : state_(state) {}

bool Sensors::accept_source(SensorSource current, SensorSource incoming) const
{
    return syslib::data_model::source_priority(incoming) <= syslib::data_model::source_priority(current);
}

void Sensors::poll(uint64_t now_us)
{
    (void)now_us;
    update_apb_command();
    // TODO: translate source timeout/priority model from sensors.py.
    // TODO: translate GPS/wind/water/rudder update filters and true wind synthesis.
}

void Sensors::update_apb_command()
{
    if (!state_.navigation.apb.mode_hint) return;
    // TODO: preserve original APB 2 Hz gate and nav-mode/engaged checks.
    // TODO: heading_command = track + apb.xte.gain * xte when ap.mode == nav.
}

} // namespace pypilot
