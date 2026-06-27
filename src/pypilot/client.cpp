#include "pypilot/pypilot.hpp"

namespace pypilot {

PypilotClient::PypilotClient(EventLoop& loop, PypilotState& state, ValueRegistry& values)
    : loop_(loop), state_(state), values_(values)
{
    (void)state_;
    (void)values_;
}

void PypilotClient::poll()
{
    // TODO: translate client.py nonblocking connect, reconnect, watch resend,
    // and incoming line handling.
    loop_.run_once();
}

void PypilotClient::receive(double timeout_seconds)
{
    poll();
    if (timeout_seconds > 0.0) loop_.sleep_us(seconds_to_us(timeout_seconds));
}

} // namespace pypilot
