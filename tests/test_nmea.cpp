#include "pypilot/pypilot.hpp"
#include "pypilot/syslib/nmea0183.hpp"

#include <cassert>

int main()
{
    pypilot::EventLoop loop;
    pypilot::PypilotState state;
    pypilot::Nmea nmea(loop, state);

    assert(nmea.feed_line("$HCHDT,123.4,T*2B", pypilot::SensorSource::serial));
    assert(state.imu.valid);
    assert(state.imu.heading > 123.3f && state.imu.heading < 123.5f);

    pypilot::syslib::nmea0183::Sentence sentence;
    assert(pypilot::syslib::nmea0183::parse_line("$HCHDT,123.4,T*2B", sentence));
    assert(sentence.type == "HDT");
    return 0;
}
