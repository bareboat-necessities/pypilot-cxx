#pragma once

#include "pypilot/syslib/event_loop.hpp"

namespace pypilot::syslib::event_loop {

using ArduinoEventLoop = EventLoop<>;

// TODO: replace std::chrono/std::thread behavior with micros()/delayMicroseconds()
// when compiling under ARDUINO.
// TODO: fold in the uploaded Arduino Serial/WiFi stream backend.

} // namespace pypilot::syslib::event_loop
