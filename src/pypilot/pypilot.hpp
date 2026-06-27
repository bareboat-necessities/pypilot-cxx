#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
    enum class Kind { number, boolean, string, json };
    enum class FieldId {
        server_running,
        server_port,
        ap_enabled,
        ap_mode,
        ap_pilot,
        ap_heading,
        ap_heading_command,
        ap_heading_error,
        ap_command,
        ap_tack_state,
        ap_tack_direction,
        imu_heading,
        imu_pitch,
        imu_roll,
        imu_heading_rate,
        gps_speed,
        gps_track,
        gps_latitude,
        gps_longitude,
        wind_direction,
        wind_speed,
        truewind_direction,
        truewind_speed,
        water_speed,
        water_leeway,
        rudder_angle,
        rudder_offset,
        rudder_range,
        servo_command,
        servo_current,
        servo_voltage,
        servo_engaged,
        servo_fault,
        apb_xte,
        apb_track,
        apb_sender,
        apb_mode,
    };

    struct Info {
        const char* type = "Value";
        bool writable = false;
        bool persistent = false;
        bool profiled = false;
        bool directional = false;
        bool has_min = false;
        bool has_max = false;
        double min_value = 0.0;
        double max_value = 0.0;
        const char* units = "";
        const char* choices_csv = "";
    };

    struct Descriptor {
        const char* name;
        FieldId id;
        Kind kind;
        Info info;
    };

    void attach(PypilotState& state);
    bool attached() const { return state_ != nullptr; }

    bool set_number(std::string_view name, double value);
    bool set_bool(std::string_view name, bool value);
    bool set_string(std::string_view name, std::string_view value);
    bool set_json(std::string_view name, std::string_view json);
    bool set_from_wire(std::string_view name, std::string_view wire_value, bool external_write = true);

    std::optional<double> get_number(std::string_view name) const;
    std::optional<bool> get_bool(std::string_view name) const;
    std::optional<std::string> get_string(std::string_view name) const;
    std::optional<std::string> get_wire(std::string_view name) const;

    const Descriptor* find(std::string_view name) const;
    std::vector<std::string> names() const;
    std::string values_json() const;
    std::string info_json(std::string_view name) const;

    bool watch(std::string_view name, double period_seconds, uint64_t now_us, std::string* initial_line = nullptr);
    bool unwatch(std::string_view name);
    std::vector<std::string> take_due_watches(uint64_t now_us);
    std::vector<std::string> take_changed_watches();

private:
    struct Watch {
        FieldId id;
        double period_seconds = 0.0;
        uint64_t next_due_us = 0;
    };

    bool set_number(FieldId id, double value, bool external_write);
    bool set_bool(FieldId id, bool value, bool external_write);
    bool set_string(FieldId id, std::string_view value, bool external_write);
    std::optional<double> get_number(FieldId id) const;
    std::optional<bool> get_bool(FieldId id) const;
    std::optional<std::string> get_string(FieldId id) const;
    std::optional<std::string> get_wire(FieldId id) const;
    void mark_changed(FieldId id);
    const Descriptor* descriptor(FieldId id) const;
    static std::string quote_json(std::string_view text);

    PypilotState* state_ = nullptr;
    std::vector<Watch> watches_;
    std::vector<FieldId> changed_;
};

class PypilotServer {
public:
    PypilotServer(EventLoop& loop, PypilotState& state, ValueRegistry& values);

    void poll();
    bool running() const { return running_; }

    std::vector<std::string> handle_line(std::string_view line);
    std::vector<std::string> collect_output();

private:
    std::vector<std::string> handle_assignment(std::string_view name, std::string_view value);
    std::vector<std::string> handle_watch(std::string_view expression);
    std::vector<std::string> error(std::string message) const;

    EventLoop& loop_;
    PypilotState& state_;
    ValueRegistry& values_;
    bool running_ = false;
    std::deque<std::string> output_;
};

class PypilotClient {
public:
    PypilotClient(EventLoop& loop, PypilotState& state, ValueRegistry& values);

    void poll();
    void receive(double timeout_seconds);
    void watch(std::string name, double period_seconds = 0.0);
    void set(std::string name, std::string wire_value);
    std::optional<std::string> pop_outgoing();
    void handle_line(std::string_view line);

private:
    EventLoop& loop_;
    PypilotState& state_;
    ValueRegistry& values_;
    std::deque<std::string> outgoing_;
    std::deque<std::string> incoming_;
};

class Sensors {
public:
    explicit Sensors(PypilotState& state);
    void poll(uint64_t now_us);
    bool accept_source(SensorSource current, SensorSource incoming) const;
    void update_apb_command();
private:
    PypilotState& state_;
    uint64_t last_apb_update_us_ = 0;
};

class BoatIMU {
public:
    BoatIMU(EventLoop& loop, PypilotState& state);
    bool read();
    void poll(bool enabled);
    double rate() const { return rate_hz_; }
private:
    EventLoop& loop_;
    PypilotState& state_;
    double rate_hz_ = 100.0;
};

class Servo {
public:
    Servo(EventLoop& loop, PypilotState& state);
    void command(double value);
    void set_autopilot_enabled(bool enabled);
    void poll();
private:
    EventLoop& loop_;
    PypilotState& state_;
};

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
