#include "pypilot/pypilot.hpp"

#include <cassert>
#include <string>

int main()
{
    pypilot::EventLoop loop;
    pypilot::PypilotState state;
    pypilot::ValueRegistry values;
    pypilot::PypilotServer server(loop, state, values);

    auto list = server.handle_line("list");
    assert(list.size() == 1);
    assert(list[0].find("values=") == 0);

    auto set = server.handle_line("ap.enabled=true");
    assert(set.size() == 1);
    assert(state.ap.enabled);

    auto get = server.handle_line("get ap.enabled");
    assert(get.size() == 1);
    assert(get[0] == "ap.enabled=true\n");

    auto denied = server.handle_line("imu.heading=20");
    assert(denied.size() == 1);
    assert(denied[0].find("error=") == 0);

    auto watch = server.handle_line("watch ap.heading_command");
    assert(watch.size() == 1);
    server.handle_line("ap.heading_command=44");
    auto out = server.collect_output();
    bool saw = false;
    for (const auto& line : out) if (line.find("ap.heading_command=") == 0) saw = true;
    assert(saw);
    return 0;
}
