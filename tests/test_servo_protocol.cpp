#include "pypilot/pypilot.hpp"
#include "pypilot/syslib/servo_protocol.hpp"

#include <cassert>

int main()
{
    auto pkt = pypilot::syslib::servo_protocol::encode_command(0.0);
    pypilot::syslib::servo_protocol::Packet decoded;
    assert(pypilot::syslib::servo_protocol::decode(pkt.data(), decoded));
    assert(decoded.value == 1000);

    pypilot::EventLoop loop;
    pypilot::PypilotState state;
    pypilot::Rudder rudder(state);
    state.rudder.last_device = "servo";
    assert(rudder.update(0.1, "servo", loop.now_us()));
    return 0;
}
