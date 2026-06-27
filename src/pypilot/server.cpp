#include "pypilot/pypilot.hpp"

namespace pypilot {

PypilotServer::PypilotServer(EventLoop& loop, PypilotState& state, ValueRegistry& values)
    : loop_(loop), state_(state), values_(values)
{
    state_.server.running = true;
    running_ = true;
    values_.set_bool("server.running", true);
    values_.set_number("server.port", state_.server.port);
}

void PypilotServer::poll()
{
    loop_.run_once();
    // TODO: translate server.py TCP accept/read/write/watch behavior.
    // TODO: implement JSON request handling: get, set, watch, list.
    // TODO: implement periodic persistence flush and watch output queues.
    // TODO: use syslib event loop TCP backend when folded in.
}

} // namespace pypilot
