#include "pypilot/pypilot.hpp"

#include <cassert>
#include <string>

int main()
{
    pypilot::EventLoop loop;
    pypilot::PypilotState state;
    pypilot::ValueRegistry values;
    pypilot::PypilotServer server(loop, state, values);

    std::string out;
    pypilot::StringValueWriter writer(out);
    assert(server.handle_line("watch={\"values\":true}", writer));
    assert(out.find("values=") == 0);

    out.clear();
    assert(server.handle_line("ap.enabled=true", writer));
    assert(out.empty());
    assert(state.ap.enabled);

    out.clear();
    assert(server.handle_line("watch={\"ap.heading_command\":true}", writer));
    assert(out.find("ap.heading_command=") == 0);

    out.clear();
    assert(server.handle_line("ap.heading_command=44", writer));
    assert(out.empty());
    std::string emitted = server.collect_output();
    assert(emitted.find("ap.heading_command=") != std::string::npos);
    return 0;
}
