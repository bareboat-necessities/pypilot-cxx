#include "pypilot/resolv.hpp"

#include <cassert>
#include <cmath>

int main()
{
    assert(pypilot::resolv(181.0) == -179.0);
    assert(pypilot::resolv(-181.0) == 179.0);
    assert(pypilot::heading_error(10.0, 350.0) == 20.0);
    assert(pypilot::heading_error(350.0, 10.0) == -20.0);
    assert(std::fabs(pypilot::compass_normalize(-1.0) - 359.0) < 1e-9);
    return 0;
}
