#include "pypilot/pypilot.hpp"

namespace pypilot {

PypilotClient::PypilotClient(EventLoop& loop, PypilotState& state, ValueRegistry& values)
    : loop_(loop), state_(state), values_(values)
{
    values_.attach(state_);
}

void PypilotClient::poll()
{
    loop_.run_once();
    // TODO: translate client.py connection and value subscription behavior.
}

void PypilotClient::receive(double timeout_seconds)
{
    poll();
    if (timeout_seconds > 0.0) loop_.sleep_us(seconds_to_us(timeout_seconds));
}

void PypilotClient::watch(std::string name, double period_seconds)
{
    outgoing_.push_back(std::string("watch ") + name + "=" + std::to_string(period_seconds) + "\n");
}

void PypilotClient::set(std::string name, std::string wire_value)
{
    outgoing_.push_back(name + "=" + wire_value + "\n");
}

std::optional<std::string> PypilotClient::pop_outgoing()
{
    if (outgoing_.empty()) return std::nullopt;
    std::string out = outgoing_.front();
    outgoing_.pop_front();
    return out;
}

void PypilotClient::handle_line(std::string_view line)
{
    const size_t eq = line.find('=');
    if (eq == std::string_view::npos) return;
    values_.set_from_wire(line.substr(0, eq), line.substr(eq + 1), false);
}

} // namespace pypilot
