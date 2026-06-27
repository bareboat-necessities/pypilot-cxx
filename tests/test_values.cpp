#include "pypilot/pypilot.hpp"

#include <cassert>
#include <string>

int main()
{
    pypilot::PypilotState state;
    pypilot::ValueRegistry values;
    values.attach(state);

    assert(values.set_real("ap.heading", 12.5f));
    assert(values.set_bool("ap.enabled", true));
    assert(values.set_string("ap.mode", "compass"));

    pypilot::Real r = 0;
    bool b = false;
    std::string_view s;
    assert(values.read_real("ap.heading", r) && r == 12.5f);
    assert(values.read_bool("ap.enabled", b) && b);
    assert(values.read_string("ap.mode", s) && s == "compass");

    std::string line;
    pypilot::StringValueWriter writer(line);
    assert(values.write_value_line("ap.heading", writer));
    assert(line.find("ap.heading=") == 0);

    std::string initial;
    pypilot::StringValueWriter initial_writer(initial);
    assert(values.watch("ap.heading_command", 0, 0, &initial_writer));
    assert(!initial.empty());
    values.set_real("ap.heading_command", 21);
    std::string changed;
    pypilot::StringValueWriter changed_writer(changed);
    values.emit_changed_watches(changed_writer);
    assert(changed.find("ap.heading_command=") != std::string::npos);
    return 0;
}
