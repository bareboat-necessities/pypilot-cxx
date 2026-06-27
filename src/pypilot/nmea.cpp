#include "pypilot/pypilot.hpp"
#include "pypilot/syslib/nmea0183.hpp"

namespace pypilot {

Nmea::Nmea(EventLoop& loop, PypilotState& state) : loop_(loop), state_(state) {}

bool Nmea::feed_line(std::string_view line, SensorSource source)
{
    syslib::nmea0183::Sentence sentence;
    if (!syslib::nmea0183::parse_line(line, sentence)) return false;
    return syslib::nmea0183::apply_sentence(sentence, state_, loop_.now_us(), source);
}

void Nmea::poll()
{
    // TODO: translate nmea.py bridge behavior without multiprocessing.
    // TODO: implement serial/TCP/UDP line feeds using syslib event-loop streams.
    // TODO: implement NMEA output rate limiting and output sentence formatters.
}

} // namespace pypilot
