#include "pypilot/pypilot.hpp"

#include <cassert>
#include <string>

int main()
{
    pypilot::PypilotState state;
    pypilot::ValueRegistry values;
    values.attach(state);

    assert(values.set_number("ap.heading", 12.5));
    assert(values.set_bool("ap.enabled", true));
    assert(values.set_string("ap.mode", "compass"));

    assert(state.ap.heading == 12.5f);
    assert(state.ap.enabled);
    assert(state.ap.mode == "compass");

    assert(values.get_number("ap.heading").value() == 12.5);
    assert(values.get_bool("ap.enabled").value());
    assert(values.get_string("ap.mode").value() == "compass");
    assert(!values.set_from_wire("ap.heading", "20", true));
    assert(values.set_from_wire("ap.heading_command", "20", true));
    assert(state.ap.heading_command == 20.0f);
    assert(values.names().size() >= 20);
    assert(values.values_json().find("ap.enabled") != std::string::npos);

    std::string initial;
    assert(values.watch("ap.heading_command", 0.0, 0, &initial));
    assert(!initial.empty());
    values.set_number("ap.heading_command", 21);
    auto changed = values.take_changed_watches();
    assert(changed.size() == 1);
    assert(changed[0].find("ap.heading_command=") == 0);
    return 0;
}
