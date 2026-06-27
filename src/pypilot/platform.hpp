#pragma once

#include <cstdint>
#include "pypilot/syslib/event_loop.hpp"

namespace pypilot {

using EventLoop = syslib::event_loop::EventLoop<>;

inline double monotonic_seconds(const EventLoop& loop)
{
    return static_cast<double>(loop.now_us()) * 1.0e-6;
}

inline uint64_t seconds_to_us(double seconds)
{
    return seconds <= 0.0 ? 0ULL : static_cast<uint64_t>(seconds * 1000000.0);
}

inline double us_to_seconds(uint64_t us)
{
    return static_cast<double>(us) * 1.0e-6;
}

} // namespace pypilot
