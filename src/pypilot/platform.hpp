#pragma once

#include <cstdint>
#include "async_event_loop.hpp"
#include "pypilot/syslib/data_model.hpp"

namespace pypilot {
using Real = syslib::data_model::Real;
using EventLoop = async_event_loop::EventLoop<>;
inline Real monotonic_seconds(const EventLoop& loop) { return static_cast<Real>(loop.now_us()) / Real{1000000}; }
inline uint64_t seconds_to_us(Real seconds) { return seconds <= Real{0} ? 0ULL : static_cast<uint64_t>(seconds * Real{1000000}); }
inline Real us_to_seconds(uint64_t us) { return static_cast<Real>(us) / Real{1000000}; }
} // namespace pypilot
