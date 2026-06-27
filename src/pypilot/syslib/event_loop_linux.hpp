#pragma once

#include "pypilot/syslib/event_loop.hpp"

namespace pypilot::syslib::event_loop {

using LinuxEventLoop = EventLoop<>;

// TODO: fold in the uploaded Linux libevent/TCP/UDP/serial stream backend.
// TODO: expose bounded poll behavior equivalent to Python select.poll(timeout).
// TODO: keep POSIX/libevent includes out of normal translated pypilot/*.cpp files.

} // namespace pypilot::syslib::event_loop
