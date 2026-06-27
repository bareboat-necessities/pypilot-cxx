#include "pypilot/pypilot.hpp"

namespace pypilot {

BoatIMU::BoatIMU(EventLoop& loop, PypilotState& state) : loop_(loop), state_(state) {}

bool BoatIMU::read()
{
    // TODO: translate boatimu.py runtime link before low-level IMU math.
    // TODO: later use Eigen for quaternion/vector state.
    state_.imu.valid = true;
    state_.imu.timestamp_us = loop_.now_us();
    state_.ap.heading = state_.imu.heading;
    return true;
}

void BoatIMU::poll(bool enabled)
{
    (void)enabled;
    // TODO: translate calibration/background maintenance polling.
}

} // namespace pypilot
