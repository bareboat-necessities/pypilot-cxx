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

class ValueWriter {
public:
    virtual ~ValueWriter() = default;
    virtual bool write(std::string_view text) = 0;

    bool write_char(char c) { return write(std::string_view(&c, 1)); }
    bool write_bool(bool value) { return write(value ? "true" : "false"); }
    bool write_real(Real value);
    bool write_u32(uint32_t value);
    bool write_json_string(std::string_view text);
};

class StringValueWriter final : public ValueWriter {
public:
    explicit StringValueWriter(std::string& out) : out_(out) {}
    bool write(std::string_view text) override { out_.append(text.data(), text.size()); return true; }
private:
    std::string& out_;
};

class ValueRegistry {
public:
    static constexpr size_t MaxWatches = 64;

    enum class Kind : uint8_t { real, boolean, string };

    enum class FieldId : uint16_t {
        server_running,
        server_port,
        ap_enabled,
        ap_mode,
        ap_pilot,
        ap_heading,
        ap_heading_command,
        ap_heading_error,
        ap_command,
        rudder_angle,
        rudder_offset,
        rudder_range,
        servo_command,
        servo_current,
        servo_voltage,
        servo_engaged,
        servo_fault,
        count,
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

    struct Descriptor {
        const char* name;
        FieldId id;
        Kind kind;
        Info info;
    };

    void attach(PypilotState& state) { state_ = &state; }
    bool attached() const { return state_ != nullptr; }

    const Descriptor* find(std::string_view name) const;
    const Descriptor* descriptor(FieldId id) const;
    static constexpr size_t FieldCount = static_cast<size_t>(FieldId::count);
    static constexpr size_t field_count() { return FieldCount; }

    bool set_from_wire(std::string_view name, std::string_view wire_value, bool external_write = true);
    bool set_real(std::string_view name, Real value, bool external_write = false);
    bool set_number(std::string_view name, Real value) { return set_real(name, value); }
    bool set_bool(std::string_view name, bool value, bool external_write = false);
    bool set_string(std::string_view name, std::string_view value, bool external_write = false);

    bool read_real(std::string_view name, Real& out) const;
    bool read_bool(std::string_view name, bool& out) const;
    bool read_string(std::string_view name, std::string_view& out) const;

    bool write_value(std::string_view name, ValueWriter& out) const;
    bool write_value_line(std::string_view name, ValueWriter& out) const;
    bool write_value_line(FieldId id, ValueWriter& out) const;
    bool write_values_json(ValueWriter& out) const;
    bool write_info_json(FieldId id, ValueWriter& out) const;

    bool watch(std::string_view name, Real period_seconds, uint64_t now_us, ValueWriter* initial_line = nullptr);
    bool unwatch(std::string_view name);
    void mark_dirty(FieldId id);
    void emit_changed_watches(ValueWriter& out);
    void emit_due_watches(uint64_t now_us, ValueWriter& out);

private:
    struct Watch {
        bool active = false;
        FieldId id = FieldId::server_running;
        Real period_seconds = 0;
        uint64_t next_due_us = 0;
    };

    bool set_real(FieldId id, Real value, bool external_write);
    bool set_bool(FieldId id, bool value, bool external_write);
    bool set_string(FieldId id, std::string_view value, bool external_write);
    bool read_real(FieldId id, Real& out) const;
    bool read_bool(FieldId id, bool& out) const;
    bool read_string(FieldId id, std::string_view& out) const;

    PypilotState* state_ = nullptr;
    std::array<Watch, MaxWatches> watches_{};
    std::array<bool, FieldCount> dirty_{};
};

class PypilotServer {
public:
    PypilotServer(EventLoop& loop, PypilotState& state, ValueRegistry& values);

    void poll();
    void poll(ValueWriter& out);
    bool running() const { return running_; }

    bool handle_line(std::string_view line, ValueWriter& out);
    std::string handle_line(std::string_view line);
    std::string collect_output();

private:
    bool handle_assignment(std::string_view name, std::string_view value, ValueWriter& out);
    bool handle_watch(std::string_view expression, ValueWriter& out);
    bool error(ValueWriter& out, std::string_view message) const;

    EventLoop& loop_;
    PypilotState& state_;
    ValueRegistry& values_;
    bool running_ = false;
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
    EventLoop& loop_;
    PypilotState& state_;
    ValueRegistry& values_;
    std::deque<std::string> outgoing_;
    std::deque<std::string> incoming_;
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
