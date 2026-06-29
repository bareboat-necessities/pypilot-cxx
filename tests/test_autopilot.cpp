#include "pypilot/pypilot.hpp"

#include <cassert>

int main()
{
    pypilot::EventLoop loop;
    pypilot::PypilotState state;
    pypilot::Autopilot autopilot(loop, state);

    state.imu.heading = 350.0f;
    state.ap.heading = 350.0f;
    state.ap.heading_command = 10.0f;
    state.ap.enabled = true;
    autopilot.iteration();

    assert(state.ap.heading_error > 19.0f && state.ap.heading_error < 21.0f);
    assert(state.servo.engaged);
    return 0;
}
