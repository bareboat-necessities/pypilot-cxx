#include "pypilot/pypilot.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace pypilot {
namespace {
using FieldId = ValueRegistry::FieldId;
using Kind = ValueRegistry::Kind;
using Descriptor = ValueRegistry::Descriptor;

constexpr Descriptor kFields[] = {
    {"server.running", FieldId::server_running, Kind::boolean, {"BooleanValue", false}},
    {"server.port", FieldId::server_port, Kind::number, {"Value", false}},
    {"ap.enabled", FieldId::ap_enabled, Kind::boolean, {"BooleanProperty", true, true}},
    {"ap.mode", FieldId::ap_mode, Kind::string, {"EnumProperty", true, true, false, false, false, false, 0, 0, "", "compass,gps,wind,true wind"}},
    {"ap.pilot", FieldId::ap_pilot, Kind::string, {"EnumProperty", true, true, false, false, false, false, 0, 0, "", "basic,simple,gps,wind,absolute,rate,deadzone,vmg"}},
    {"ap.heading", FieldId::ap_heading, Kind::number, {"SensorValue", false, false, false, true}},
    {"ap.heading_command", FieldId::ap_heading_command, Kind::number, {"RangeProperty", true, false, false, true, true, true, 0, 360, "degrees"}},
    {"ap.heading_error", FieldId::ap_heading_error, Kind::number, {"SensorValue", false, false, false, true}},
    {"ap.command", FieldId::ap_command, Kind::number, {"SensorValue", false, false, false, false, true, true, -1, 1}},
    {"ap.tack.state", FieldId::ap_tack_state, Kind::boolean, {"BooleanProperty", true}},
    {"ap.tack.direction", FieldId::ap_tack_direction, Kind::number, {"RangeProperty", true, false, false, false, true, true, -1, 1}},
    {"imu.heading", FieldId::imu_heading, Kind::number, {"SensorValue", false, false, false, true}},
    {"imu.pitch", FieldId::imu_pitch, Kind::number, {"SensorValue", false}},
    {"imu.roll", FieldId::imu_roll, Kind::number, {"SensorValue", false}},
    {"imu.headingrate", FieldId::imu_heading_rate, Kind::number, {"SensorValue", false}},
    {"gps.speed", FieldId::gps_speed, Kind::number, {"SensorValue", false, false, false, false, true, false, 0, 0, "knots"}},
    {"gps.track", FieldId::gps_track, Kind::number, {"SensorValue", false, false, false, true}},
    {"gps.latitude", FieldId::gps_latitude, Kind::number, {"SensorValue", false}},
    {"gps.longitude", FieldId::gps_longitude, Kind::number, {"SensorValue", false}},
    {"wind.direction", FieldId::wind_direction, Kind::number, {"SensorValue", false, false, false, true}},
    {"wind.speed", FieldId::wind_speed, Kind::number, {"SensorValue", false, false, false, false, true, false, 0, 0, "knots"}},
    {"truewind.direction", FieldId::truewind_direction, Kind::number, {"SensorValue", false, false, false, true}},
    {"truewind.speed", FieldId::truewind_speed, Kind::number, {"SensorValue", false, false, false, false, true, false, 0, 0, "knots"}},
    {"water.speed", FieldId::water_speed, Kind::number, {"SensorValue", false, false, false, false, true, false, 0, 0, "knots"}},
    {"water.leeway", FieldId::water_leeway, Kind::number, {"SensorValue", false}},
    {"rudder.angle", FieldId::rudder_angle, Kind::number, {"SensorValue", false, false, false, true}},
    {"rudder.offset", FieldId::rudder_offset, Kind::number, {"RangeProperty", true, true, false, true, true, true, -45, 45, "degrees"}},
    {"rudder.range", FieldId::rudder_range, Kind::number, {"RangeProperty", true, true, false, false, true, true, 1, 90, "degrees"}},
    {"servo.command", FieldId::servo_command, Kind::number, {"RangeProperty", true, false, false, false, true, true, -1, 1}},
    {"servo.current", FieldId::servo_current, Kind::number, {"SensorValue", false, false, false, false, true, false, 0, 0, "amps"}},
    {"servo.voltage", FieldId::servo_voltage, Kind::number, {"SensorValue", false, false, false, false, true, false, 0, 0, "volts"}},
    {"servo.engaged", FieldId::servo_engaged, Kind::boolean, {"BooleanValue", false}},
    {"servo.fault", FieldId::servo_fault, Kind::boolean, {"BooleanValue", false}},
    {"apb.xte", FieldId::apb_xte, Kind::number, {"SensorValue", false, false, false, false, true, true, -0.15, 0.15, "nmi"}},
    {"apb.track", FieldId::apb_track, Kind::number, {"SensorValue", false, false, false, true}},
    {"apb.sender", FieldId::apb_sender, Kind::string, {"StringValue", false}},
    {"apb.mode", FieldId::apb_mode, Kind::boolean, {"BooleanValue", false}},
};

std::string trim_copy(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.remove_suffix(1);
    return std::string(s);
}

bool parse_bool(std::string_view s, bool& out)
{
    const std::string v = trim_copy(s);
    if (v == "true" || v == "True" || v == "1") { out = true; return true; }
    if (v == "false" || v == "False" || v == "0") { out = false; return true; }
    return false;
}

bool parse_number(std::string_view s, double& out)
{
    const std::string v = trim_copy(s);
    if (v.empty()) return false;
    char* end = nullptr;
    out = std::strtod(v.c_str(), &end);
    return end != v.c_str();
}

std::string unquote(std::string_view s)
{
    std::string v = trim_copy(s);
    if (v.size() >= 2 && v.front() == '"' && v.back() == '"') v = v.substr(1, v.size() - 2);
    return v;
}

uint64_t period_to_us(double seconds)
{
    return seconds <= 0.0 ? 0ULL : static_cast<uint64_t>(seconds * 1000000.0);
}
} // namespace

void ValueRegistry::attach(PypilotState& state) { state_ = &state; }

const Descriptor* ValueRegistry::descriptor(FieldId id) const
{
    for (const auto& d : kFields) if (d.id == id) return &d;
    return nullptr;
}

const Descriptor* ValueRegistry::find(std::string_view name) const
{
    for (const auto& d : kFields) if (name == d.name) return &d;
    return nullptr;
}

std::vector<std::string> ValueRegistry::names() const
{
    std::vector<std::string> out;
    out.reserve(sizeof(kFields) / sizeof(kFields[0]));
    for (const auto& d : kFields) out.emplace_back(d.name);
    return out;
}

bool ValueRegistry::set_number(std::string_view name, double value)
{
    const auto* d = find(name);
    return d && set_number(d->id, value, false);
}

bool ValueRegistry::set_bool(std::string_view name, bool value)
{
    const auto* d = find(name);
    return d && set_bool(d->id, value, false);
}

bool ValueRegistry::set_string(std::string_view name, std::string_view value)
{
    const auto* d = find(name);
    return d && set_string(d->id, value, false);
}

bool ValueRegistry::set_json(std::string_view name, std::string_view json)
{
    return set_string(name, json);
}

bool ValueRegistry::set_from_wire(std::string_view name, std::string_view wire_value, bool external_write)
{
    const auto* d = find(trim_copy(name));
    if (!d || !state_) return false;
    if (external_write && !d->info.writable) return false;
    switch (d->kind) {
    case Kind::number: { double v = 0; return parse_number(wire_value, v) && set_number(d->id, v, external_write); }
    case Kind::boolean: { bool v = false; return parse_bool(wire_value, v) && set_bool(d->id, v, external_write); }
    case Kind::string:
    case Kind::json: return set_string(d->id, unquote(wire_value), external_write);
    }
    return false;
}

bool ValueRegistry::set_number(FieldId id, double value, bool external_write)
{
    (void)external_write;
    if (!state_) return false;
    const auto* d = descriptor(id);
    if (!d || d->kind != Kind::number) return false;
    if (d->info.has_min && value < d->info.min_value) return false;
    if (d->info.has_max && value > d->info.max_value) return false;

    auto assign = [&](Real& ref) { if (std::fabs(static_cast<double>(ref) - value) < 1e-9) return true; ref = static_cast<Real>(value); mark_changed(id); return true; };
    auto assign_source = [&](auto& stamped) { if (stamped.valid && std::fabs(static_cast<double>(stamped.value) - value) < 1e-9) return true; stamped.value = static_cast<Real>(value); stamped.valid = true; mark_changed(id); return true; };

    switch (id) {
    case FieldId::server_port: if (state_->server.port != static_cast<uint16_t>(value)) { state_->server.port = static_cast<uint16_t>(value); mark_changed(id); } return true;
    case FieldId::ap_heading: return assign(state_->ap.heading);
    case FieldId::ap_heading_command: return assign(state_->ap.heading_command);
    case FieldId::ap_heading_error: return assign(state_->ap.heading_error);
    case FieldId::ap_command: return assign(state_->ap.command);
    case FieldId::ap_tack_direction: if (state_->ap.tack.direction != static_cast<int>(value)) { state_->ap.tack.direction = static_cast<int>(value); mark_changed(id); } return true;
    case FieldId::imu_heading: return assign(state_->imu.heading);
    case FieldId::imu_pitch: return assign(state_->imu.pitch);
    case FieldId::imu_roll: return assign(state_->imu.roll);
    case FieldId::imu_heading_rate: return assign(state_->imu.heading_rate);
    case FieldId::gps_speed: return assign_source(state_->gps.speed);
    case FieldId::gps_track: return assign_source(state_->gps.track);
    case FieldId::gps_latitude: return assign_source(state_->gps.latitude);
    case FieldId::gps_longitude: return assign_source(state_->gps.longitude);
    case FieldId::wind_direction: return assign_source(state_->wind.apparent_direction);
    case FieldId::wind_speed: return assign_source(state_->wind.apparent_speed);
    case FieldId::truewind_direction: return assign_source(state_->wind.true_direction);
    case FieldId::truewind_speed: return assign_source(state_->wind.true_speed);
    case FieldId::water_speed: return assign_source(state_->water.speed);
    case FieldId::water_leeway: return assign_source(state_->water.leeway);
    case FieldId::rudder_angle: return assign_source(state_->rudder.angle);
    case FieldId::rudder_offset: return assign(state_->rudder.offset);
    case FieldId::rudder_range: return assign(state_->rudder.range);
    case FieldId::servo_command: return assign(state_->servo.command);
    case FieldId::servo_current: return assign(state_->servo.current);
    case FieldId::servo_voltage: return assign(state_->servo.voltage);
    case FieldId::apb_xte: return assign_source(state_->navigation.apb.xte_nmi);
    case FieldId::apb_track: return assign_source(state_->navigation.apb.track_deg);
    default: return false;
    }
}

bool ValueRegistry::set_bool(FieldId id, bool value, bool external_write)
{
    (void)external_write;
    if (!state_) return false;
    const auto* d = descriptor(id);
    if (!d || d->kind != Kind::boolean) return false;
    auto assign = [&](bool& ref) { if (ref != value) { ref = value; mark_changed(id); } return true; };
    switch (id) {
    case FieldId::server_running: return assign(state_->server.running);
    case FieldId::ap_enabled: return assign(state_->ap.enabled);
    case FieldId::ap_tack_state: return assign(state_->ap.tack.state);
    case FieldId::servo_engaged: return assign(state_->servo.engaged);
    case FieldId::servo_fault: return assign(state_->servo.fault);
    case FieldId::apb_mode: return assign(state_->navigation.apb.mode_hint);
    default: return false;
    }
}

bool ValueRegistry::set_string(FieldId id, std::string_view value, bool external_write)
{
    (void)external_write;
    if (!state_) return false;
    const auto* d = descriptor(id);
    if (!d || (d->kind != Kind::string && d->kind != Kind::json)) return false;
    const std::string v(value);
    auto assign = [&](std::string& ref) { if (ref != v) { ref = v; mark_changed(id); } return true; };
    switch (id) {
    case FieldId::ap_mode: return assign(state_->ap.mode);
    case FieldId::ap_pilot: return assign(state_->ap.pilot);
    case FieldId::apb_sender: return assign(state_->navigation.apb.sender_id);
    default: return false;
    }
}

std::optional<double> ValueRegistry::get_number(std::string_view name) const
{
    const auto* d = find(name);
    return d ? get_number(d->id) : std::nullopt;
}

std::optional<bool> ValueRegistry::get_bool(std::string_view name) const
{
    const auto* d = find(name);
    return d ? get_bool(d->id) : std::nullopt;
}

std::optional<std::string> ValueRegistry::get_string(std::string_view name) const
{
    const auto* d = find(name);
    return d ? get_string(d->id) : std::nullopt;
}

std::optional<double> ValueRegistry::get_number(FieldId id) const
{
    if (!state_) return std::nullopt;
    switch (id) {
    case FieldId::server_port: return state_->server.port;
    case FieldId::ap_heading: return state_->ap.heading;
    case FieldId::ap_heading_command: return state_->ap.heading_command;
    case FieldId::ap_heading_error: return state_->ap.heading_error;
    case FieldId::ap_command: return state_->ap.command;
    case FieldId::ap_tack_direction: return state_->ap.tack.direction;
    case FieldId::imu_heading: return state_->imu.heading;
    case FieldId::imu_pitch: return state_->imu.pitch;
    case FieldId::imu_roll: return state_->imu.roll;
    case FieldId::imu_heading_rate: return state_->imu.heading_rate;
    case FieldId::gps_speed: return state_->gps.speed.value;
    case FieldId::gps_track: return state_->gps.track.value;
    case FieldId::gps_latitude: return state_->gps.latitude.value;
    case FieldId::gps_longitude: return state_->gps.longitude.value;
    case FieldId::wind_direction: return state_->wind.apparent_direction.value;
    case FieldId::wind_speed: return state_->wind.apparent_speed.value;
    case FieldId::truewind_direction: return state_->wind.true_direction.value;
    case FieldId::truewind_speed: return state_->wind.true_speed.value;
    case FieldId::water_speed: return state_->water.speed.value;
    case FieldId::water_leeway: return state_->water.leeway.value;
    case FieldId::rudder_angle: return state_->rudder.angle.value;
    case FieldId::rudder_offset: return state_->rudder.offset;
    case FieldId::rudder_range: return state_->rudder.range;
    case FieldId::servo_command: return state_->servo.command;
    case FieldId::servo_current: return state_->servo.current;
    case FieldId::servo_voltage: return state_->servo.voltage;
    case FieldId::apb_xte: return state_->navigation.apb.xte_nmi.value;
    case FieldId::apb_track: return state_->navigation.apb.track_deg.value;
    default: return std::nullopt;
    }
}

std::optional<bool> ValueRegistry::get_bool(FieldId id) const
{
    if (!state_) return std::nullopt;
    switch (id) {
    case FieldId::server_running: return state_->server.running;
    case FieldId::ap_enabled: return state_->ap.enabled;
    case FieldId::ap_tack_state: return state_->ap.tack.state;
    case FieldId::servo_engaged: return state_->servo.engaged;
    case FieldId::servo_fault: return state_->servo.fault;
    case FieldId::apb_mode: return state_->navigation.apb.mode_hint;
    default: return std::nullopt;
    }
}

std::optional<std::string> ValueRegistry::get_string(FieldId id) const
{
    if (!state_) return std::nullopt;
    switch (id) {
    case FieldId::ap_mode: return state_->ap.mode;
    case FieldId::ap_pilot: return state_->ap.pilot;
    case FieldId::apb_sender: return state_->navigation.apb.sender_id;
    default: return std::nullopt;
    }
}

std::optional<std::string> ValueRegistry::get_wire(std::string_view name) const
{
    const auto* d = find(name);
    return d ? get_wire(d->id) : std::nullopt;
}

std::optional<std::string> ValueRegistry::get_wire(FieldId id) const
{
    const auto* d = descriptor(id);
    if (!d) return std::nullopt;
    std::ostringstream out;
    switch (d->kind) {
    case Kind::number: {
        auto v = get_number(id);
        if (!v) return std::nullopt;
        if (std::isnan(*v)) return std::string("\"nan\"");
        out << std::fixed << std::setprecision(4) << *v;
        return out.str();
    }
    case Kind::boolean: {
        auto v = get_bool(id);
        if (!v) return std::nullopt;
        return *v ? std::string("true") : std::string("false");
    }
    case Kind::string:
    case Kind::json: {
        auto v = get_string(id);
        if (!v) return std::nullopt;
        return quote_json(*v);
    }
    }
    return std::nullopt;
}

std::string ValueRegistry::quote_json(std::string_view text)
{
    std::string out = "\"";
    for (char c : text) {
        if (c == '\\' || c == '"') out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

std::string ValueRegistry::info_json(std::string_view name) const
{
    const auto* d = find(name);
    if (!d) return "{}";
    const auto& i = d->info;
    std::ostringstream out;
    out << "{\"type\":" << quote_json(i.type);
    if (i.writable) out << ",\"writable\":true";
    if (i.persistent) out << ",\"persistent\":true";
    if (i.profiled) out << ",\"profiled\":true";
    if (i.directional) out << ",\"directional\":true";
    if (i.has_min) out << ",\"min\":" << i.min_value;
    if (i.has_max) out << ",\"max\":" << i.max_value;
    if (i.units && i.units[0]) out << ",\"units\":" << quote_json(i.units);
    if (i.choices_csv && i.choices_csv[0]) {
        out << ",\"choices\":[";
        std::string_view choices(i.choices_csv);
        bool first = true;
        while (!choices.empty()) {
            const size_t comma = choices.find(',');
            const std::string_view item = comma == std::string_view::npos ? choices : choices.substr(0, comma);
            if (!first) out << ',';
            first = false;
            out << quote_json(item);
            if (comma == std::string_view::npos) break;
            choices.remove_prefix(comma + 1);
        }
        out << ']';
    }
    out << '}';
    return out.str();
}

std::string ValueRegistry::values_json() const
{
    std::ostringstream out;
    out << '{';
    bool first = true;
    for (const auto& d : kFields) {
        if (!first) out << ',';
        first = false;
        out << quote_json(d.name) << ':' << info_json(d.name);
    }
    out << '}';
    return out.str();
}

void ValueRegistry::mark_changed(FieldId id)
{
    if (std::find(changed_.begin(), changed_.end(), id) == changed_.end()) changed_.push_back(id);
}

bool ValueRegistry::watch(std::string_view name, double period_seconds, uint64_t now_us, std::string* initial_line)
{
    const auto* d = find(name);
    if (!d) return false;
    if (initial_line) {
        auto wire = get_wire(d->id);
        if (wire) *initial_line = std::string(d->name) + '=' + *wire + '\n';
    }
    for (auto& w : watches_) {
        if (w.id == d->id) {
            w.period_seconds = period_seconds;
            w.next_due_us = now_us + period_to_us(period_seconds);
            return true;
        }
    }
    watches_.push_back({d->id, period_seconds, now_us + period_to_us(period_seconds)});
    return true;
}

bool ValueRegistry::unwatch(std::string_view name)
{
    const auto* d = find(name);
    if (!d) return false;
    watches_.erase(std::remove_if(watches_.begin(), watches_.end(), [&](const Watch& w) { return w.id == d->id; }), watches_.end());
    return true;
}

std::vector<std::string> ValueRegistry::take_due_watches(uint64_t now_us)
{
    std::vector<std::string> out;
    for (auto& w : watches_) {
        if (w.period_seconds <= 0.0 || now_us < w.next_due_us) continue;
        const auto* d = descriptor(w.id);
        auto wire = get_wire(w.id);
        if (d && wire) out.push_back(std::string(d->name) + '=' + *wire + '\n');
        const uint64_t period = period_to_us(w.period_seconds);
        do { w.next_due_us += period; } while (period && now_us >= w.next_due_us);
    }
    return out;
}

std::vector<std::string> ValueRegistry::take_changed_watches()
{
    std::vector<std::string> out;
    for (FieldId id : changed_) {
        const bool watched = std::any_of(watches_.begin(), watches_.end(), [&](const Watch& w) { return w.id == id && w.period_seconds == 0.0; });
        if (!watched) continue;
        const auto* d = descriptor(id);
        auto wire = get_wire(id);
        if (d && wire) out.push_back(std::string(d->name) + '=' + *wire + '\n');
    }
    changed_.clear();
    return out;
}

// TODO: add profile/persistent storage behavior from values.py/server.py.
// TODO: add exact PyPilot JSON compatibility for all property metadata.
// TODO: add typed descriptors for remaining upstream value names as modules are translated.

} // namespace pypilot
