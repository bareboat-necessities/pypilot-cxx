#include "pypilot/pypilot.hpp"

#include <cassert>

int main()
{
    pypilot::ValueRegistry values;
    values.set_number("ap.heading", 12.5);
    values.set_bool("ap.enabled", true);
    values.set_string("ap.mode", "compass");

    assert(values.get_number("ap.heading").value() == 12.5);
    assert(values.get_bool("ap.enabled").value());
    assert(values.get_string("ap.mode").value() == "compass");
    assert(values.names().size() == 3);
    return 0;
}
