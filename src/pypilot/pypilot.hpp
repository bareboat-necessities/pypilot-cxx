#pragma once

#include <array>
#include <cstddef>
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
#include "pypilot/syslib/servo_protocol.hpp"

namespace pypilot {

using Real = syslib::data_model::Real;
using PypilotState = syslib::data_model::DataModel<Real>;
using SensorSource = syslib::data_model::SensorSource;

class ValueRegistry {
public:
    enum class Kind { number, boolean, string, json };
    enum class FieldId {
        server_running, server_port, ap_enabled, ap_mode, ap_pilot, ap_heading,
        ap_heading_command, ap_heading_error, ap_command, ap_tack_state,
        ap_tack_direction, imu_heading, imu_pitch, imu_roll, imu_heading_rate,
        gps_speed, gps_track, gps_latitude, gps_longitude, wind_direction,
        wind_speed, truewind_direction, truewind_speed, water_speed,
        water_leeway, rudder_angle, rudder_speed, rudder_offset, rudder_scale,
        rudder_nonlinearity, rudder_range, rudder_calibration_state,
        servo_position_command, servo_command, servo_current, servo_current_noise,
        servo_voltage, servo_controller_temp, servo_motor_temp, servo_max_current,
        servo_current_factor, servo_current_offset, servo_voltage_factor,
        servo_voltage_offset, servo_max_controller_temp, servo_max_motor_temp,
        servo_max_slew_speed, servo_max_slew_slow, servo_speed_gain, servo_gain,
        servo_clutch_pwm, servo_use_brake, servo_period, servo_compensate_current,
        servo_compensate_voltage, servo_amp_hours, servo_watts, servo_speed,
        servo_speed_min, servo_speed_max, servo_position, servo_position_p,
        servo_position_i, servo_position_d, servo_raw_command, servo_use_eeprom,
        servo_state, servo_controller, servo_flags, servo_engaged, servo_fault,
        apb_xte, apb_track, apb_sender, apb_mode,
    };
    struct Info {
        const char* type = "Value";
        bool writable = false;
        bool persistent = false;
        bool profiled = false;
        bool directional = false;
        bool has_min = false;
        bool has_max = false;
        Real min_value = 0;
        Real max_value = 0;
        const char* units = "";
        const char* choices_csv = "";
    };
    struct Descriptor { const char* name; FieldId id; Kind kind; Info info; };

    void attach(PypilotState& state);
    bool attached() const { return state_ != nullptr; }
    bool set_number(std::string_view name, Real value);
    bool set_bool(std::string_view name, bool value);
    bool set_string(std::string_view name, std::string_view value);
    bool set_json(std::string_view name, std::string_view json);
    bool set_from_wire(std::string_view name, std::string_view wire_value, bool external_write = true);
    std::optional<Real> get_number(std::string_view name) const;
    std::optional<bool> get_bool(std::string_view name) const;
    std::optional<std::string> get_string(std::string_view name) const;
    std::optional<std::string> get_wire(std::string_view name) const;
    const Descriptor* find(std::string_view name) const;
    std::vector<std::string> names() const;
    std::string values_json() const;
    std::string info_json(std::string_view name) const;
    bool watch(std::string_view name, Real period_seconds, uint64_t now_us, std::string* initial_line = nullptr);
    bool unwatch(std::string_view name);
    std::vector<std::string> take_due_watches(uint64_t now_us);
    std::vector<std::string> take_changed_watches();

private:
    struct Watch { FieldId id; Real period_seconds = 0; uint64_t next_due_us = 0; };
    bool set_number(FieldId id, Real value, bool external_write);
    bool set_bool(FieldId id, bool value, bool external_write);
    bool set_string(FieldId id, std::string_view value, bool external_write);
    std::optional<Real> get_number(FieldId id) const;
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
    EventLoop& loop_; PypilotState& state_; ValueRegistry& values_; bool running_ = false; std::deque<std::string> output_;
};

class PypilotClient {
public:
    PypilotClient(EventLoop& loop, PypilotState& state, ValueRegistry& values);
    void poll();
    void receive(Real timeout_seconds);
    void watch(std::string name, Real period_seconds = 0);
    void set(std::string name, std::string wire_value);
    std::optional<std::string> pop_outgoing();
    void handle_line(std::string_view line);
private:
    EventLoop& loop_; PypilotState& state_; ValueRegistry& values_; std::deque<std::string> outgoing_; std::deque<std::string> incoming_;
};

class Sensors { public: explicit Sensors(PypilotState& state); void poll(uint64_t now_us); bool accept_source(SensorSource current, SensorSource incoming) const; void update_apb_command(); private: PypilotState& state_; uint64_t last_apb_update_us_ = 0; };
class BoatIMU { public: BoatIMU(EventLoop& loop, PypilotState& state); bool read(); void poll(bool enabled); Real rate() const { return rate_hz_; } private: EventLoop& loop_; PypilotState& state_; Real rate_hz_ = 100; };

class Servo {
public:
    Servo(EventLoop& loop, PypilotState& state);
    void command(Real value);
    void raw_command(Real value);
    void stop();
    void reset();
    void disengage();
    void set_autopilot_enabled(bool enabled);
    void poll();
    uint16_t feed_driver_bytes(const uint8_t* bytes, size_t len);
    std::vector<uint8_t> take_driver_tx();
    bool fault() const;
private:
    void send_driver_params(int current_multiplier = 1);
    void queue_packet(const std::array<uint8_t, 4>& packet);
    void apply_driver_sample(uint16_t mask);
    EventLoop& loop_; PypilotState& state_; syslib::servo_protocol::Parser parser_; syslib::servo_protocol::TelemetrySample sample_; std::vector<uint8_t> tx_;
    int out_sync_ = 0; int lastdir_ = 0; bool ap_enabled_ = false; bool last_ap_enabled_ = false; bool disengaged_ = true;
    uint64_t command_timeout_us_ = 0; uint64_t last_zero_command_us_ = 0; uint64_t driver_timeout_start_us_ = 0;
};

class Rudder { public: explicit Rudder(PypilotState& state); void update_minmax(); bool update(Real raw, std::string_view device, uint64_t now_us); bool invalid() const; void reset(); void poll(uint64_t now_us); private: PypilotState& state_; };
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
    void compute_offsets(); void adjust_mode(); void compute_heading_error(); Pilot& selected_pilot(); void sleep_remainder(uint64_t start_us, uint64_t period_us);
    EventLoop& loop_; PypilotState& state_; ValueRegistry& values_; PypilotServer server_; PypilotClient client_; Sensors sensors_; BoatIMU boatimu_; Servo servo_; Rudder rudder_; Tacking tacking_; Nmea nmea_; std::unique_ptr<Pilot> pilot_;
};

} // namespace pypilot
