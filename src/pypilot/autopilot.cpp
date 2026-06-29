#include "pypilot/pypilot.hpp"
#include "pypilot/pilots/pilot.hpp"

namespace pypilot {

Autopilot::Autopilot(EventLoop& loop, PypilotState& state)
    : loop_(loop),
      state_(state),
      sensors_(state),
      boatimu_(loop, state),
      servo_(loop, state),
      rudder_(state),
      tacking_(state),
      nmea_(loop, state),
      pilot_(std::make_unique<BasicPilot>(state))
{
}

Autopilot::~Autopilot() = default;

void Autopilot::iteration()
{
    const uint64_t start_us = loop_.now_us();
    const uint64_t period_us = seconds_to_us(Real{1} / boatimu_.rate());

    if (!state_.ap.enabled) servo_.poll();
    sensors_.poll(start_us);

    bool imu_ok = false;
    for (int tries = 0; tries < 14; ++tries) {
        imu_ok = boatimu_.read();
        if (imu_ok) break;
        // TODO: match original autopilot.py IMU wait/retry timing exactly.
        loop_.sleep_us(period_us / 10);
    }

    compute_offsets();
    adjust_mode();
    Pilot& pilot = selected_pilot();
    pilot.compute_heading();
    compute_heading_error();
    if (!tacking_.process()) pilot.process();

    servo_.set_autopilot_enabled(state_.ap.enabled);
    if (state_.ap.enabled) servo_.poll();
    boatimu_.poll(!state_.ap.enabled);
    rudder_.poll(loop_.now_us());
    tacking_.poll(loop_.now_us());
    nmea_.poll();

    sleep_remainder(start_us, period_us);
}

void Autopilot::compute_offsets() { /* TODO: translate compute_offsets from autopilot.py. */ }
void Autopilot::adjust_mode() { /* TODO: translate mode adjustment and pilot selection from autopilot.py. */ }
void Autopilot::compute_heading_error() { state_.ap.heading_error = heading_error(state_.ap.heading_command, state_.ap.heading); }
Pilot& Autopilot::selected_pilot() { return *pilot_; /* TODO: create/select pilots by configured name instead of always basic. */ }

void Autopilot::sleep_remainder(uint64_t start_us, uint64_t period_us)
{
    const uint64_t now = loop_.now_us();
    const uint64_t elapsed = now > start_us ? now - start_us : 0;
    if (elapsed < period_us) loop_.sleep_us(period_us - elapsed);
}

} // namespace pypilot
