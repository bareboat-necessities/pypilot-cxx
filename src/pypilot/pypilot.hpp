#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pypilot/config.hpp"
#include "pypilot/platform.hpp"
#include "pypilot/resolv.hpp"
#include "pypilot/syslib/data_model.hpp"

namespace pypilot {

using Real = float;
using PypilotState = syslib::data_model::DataModel<Real>;
using SensorSource = syslib::data_model::SensorSource;

class ValueRegistry {
public:
    bool set_number(std::string name, double value);
    bool set_bool(std::string name, bool value);
    bool set_string(std::string name, std::string value);
    std::optional<double> get_number(std::string_view name) const;
    std::optional<bool> get_bool(std::string_view name) const;
    std::optional<std::string> get_string(std::string_view name) const;
    std::vector<std::string> names() const;
private:
    struct Value { enum class Kind { number, boolean, string } kind = Kind::number; double number = 0; bool boolean = false; std::string text; };
    std::unordered_map<std::string, Value> values_;
};

class PypilotServer { public: PypilotServer(EventLoop& loop, PypilotState& state, ValueRegistry& values); void poll(); bool running() const { return running_; } private: EventLoop& loop_; PypilotState& state_; ValueRegistry& values_; bool running_ = false; };
class PypilotClient { public: PypilotClient(EventLoop& loop, PypilotState& state, ValueRegistry& values); void poll(); void receive(double timeout_seconds); private: EventLoop& loop_; PypilotState& state_; ValueRegistry& values_; };
class Sensors { public: explicit Sensors(PypilotState& state); void poll(uint64_t now_us); bool accept_source(SensorSource current, SensorSource incoming) const; void update_apb_command(); private: PypilotState& state_; uint64_t last_apb_update_us_ = 0; };
class BoatIMU { public: BoatIMU(EventLoop& loop, PypilotState& state); bool read(); void poll(bool enabled); double rate() const { return rate_hz_; } private: EventLoop& loop_; PypilotState& state_; double rate_hz_ = 100.0; };
class Servo { public: Servo(EventLoop& loop, PypilotState& state); void command(double value); void set_autopilot_enabled(bool enabled); void poll(); private: EventLoop& loop_; PypilotState& state_; };
class Rudder { public: explicit Rudder(PypilotState& state); void poll(uint64_t now_us); private: PypilotState& state_; };
class Tacking { public: explicit Tacking(PypilotState& state); bool process(); void poll(uint64_t now_us); private: PypilotState& state_; };
class Nmea { public: Nmea(EventLoop& loop, PypilotState& state); bool feed_line(std::string_view line, SensorSource source = SensorSource::serial); void poll(); private: EventLoop& loop_; PypilotState& state_; };
class Pilot;

class Autopilot {
public:
    Autopilot(EventLoop& loop, PypilotState& state, ValueRegistry& values);
    ~Autopilot();
    void iteration();
    PypilotState& state() { return state_; }
    const PypilotState& state() const { return state_; }
private:
    void compute_offsets();
    void adjust_mode();
    void compute_heading_error();
    Pilot& selected_pilot();
    void sleep_remainder(uint64_t start_us, uint64_t period_us);
    EventLoop& loop_;
    PypilotState& state_;
    ValueRegistry& values_;
    PypilotServer server_;
    PypilotClient client_;
    Sensors sensors_;
    BoatIMU boatimu_;
    Servo servo_;
    Rudder rudder_;
    Tacking tacking_;
    Nmea nmea_;
    std::unique_ptr<Pilot> pilot_;
};

} // namespace pypilot
