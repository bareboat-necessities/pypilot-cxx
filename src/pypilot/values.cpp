#include "pypilot/pypilot.hpp"

#include <cstdio>
#include <cstdlib>

namespace pypilot {
namespace {
using FieldId = ValueRegistry::FieldId;
using Kind = ValueRegistry::Kind;
using Descriptor = ValueRegistry::Descriptor;

constexpr Descriptor fields[] = {
    {"server.running", FieldId::server_running, Kind::boolean, {"BooleanValue", false}},
    {"server.port", FieldId::server_port, Kind::real, {"Value", false}},
    {"ap.enabled", FieldId::ap_enabled, Kind::boolean, {"BooleanProperty", true, true}},
    {"ap.mode", FieldId::ap_mode, Kind::string, {"EnumProperty", true, true}},
    {"ap.pilot", FieldId::ap_pilot, Kind::string, {"EnumProperty", true, true}},
    {"ap.heading", FieldId::ap_heading, Kind::real, {"SensorValue", false, false, false, true}},
    {"ap.heading_command", FieldId::ap_heading_command, Kind::real, {"RangeProperty", true, false, false, true, true, true, 0, 360, "degrees"}},
    {"ap.heading_error", FieldId::ap_heading_error, Kind::real, {"SensorValue", false, false, false, true}},
    {"ap.command", FieldId::ap_command, Kind::real, {"SensorValue", false}},
    {"rudder.angle", FieldId::rudder_angle, Kind::real, {"SensorValue", false, false, false, true}},
    {"rudder.offset", FieldId::rudder_offset, Kind::real, {"RangeProperty", true, true}},
    {"rudder.range", FieldId::rudder_range, Kind::real, {"RangeProperty", true, true}},
    {"servo.command", FieldId::servo_command, Kind::real, {"RangeProperty", true, false, false, false, true, true, -1, 1}},
    {"servo.current", FieldId::servo_current, Kind::real, {"SensorValue", false}},
    {"servo.voltage", FieldId::servo_voltage, Kind::real, {"SensorValue", false}},
    {"servo.engaged", FieldId::servo_engaged, Kind::boolean, {"BooleanValue", false}},
    {"servo.fault", FieldId::servo_fault, Kind::boolean, {"BooleanValue", false}},
};

bool parse_real(std::string_view s, Real& out)
{
    char buf[32]{};
    const size_t n = s.size() < sizeof(buf) - 1 ? s.size() : sizeof(buf) - 1;
    for (size_t i = 0; i < n; ++i) buf[i] = s[i];
    char* end = nullptr;
    out = std::strtof(buf, &end);
    return end != buf;
}

bool parse_bool(std::string_view s, bool& out)
{
    if (s == "true" || s == "True" || s == "1") { out = true; return true; }
    if (s == "false" || s == "False" || s == "0") { out = false; return true; }
    return false;
}

std::string_view strip_quotes(std::string_view s)
{
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') return s.substr(1, s.size() - 2);
    return s;
}

uint64_t period_us(Real seconds)
{
    return seconds <= Real{0} ? 0 : static_cast<uint64_t>(seconds * Real{1000000});
}
} // namespace

bool ValueWriter::write_real(Real value)
{
    char buf[32];
    const int n = std::snprintf(buf, sizeof(buf), "%.4f", static_cast<double>(value));
    return n > 0 && write(std::string_view(buf, static_cast<size_t>(n)));
}

bool ValueWriter::write_u32(uint32_t value)
{
    char buf[16];
    const int n = std::snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(value));
    return n > 0 && write(std::string_view(buf, static_cast<size_t>(n)));
}

bool ValueWriter::write_json_string(std::string_view text)
{
    if (!write_char('"')) return false;
    for (char c : text) {
        if ((c == '"' || c == '\\') && !write_char('\\')) return false;
        if (!write_char(c)) return false;
    }
    return write_char('"');
}

const ValueRegistry::Descriptor* ValueRegistry::find(std::string_view name) const
{
    for (const auto& d : fields) if (name == d.name) return &d;
    return nullptr;
}

const ValueRegistry::Descriptor* ValueRegistry::descriptor(FieldId id) const
{
    for (const auto& d : fields) if (d.id == id) return &d;
    return nullptr;
}

bool ValueRegistry::set_from_wire(std::string_view name, std::string_view wire_value, bool external_write)
{
    const Descriptor* d = find(name);
    if (!d || (external_write && !d->info.writable)) return false;
    if (d->kind == Kind::real) { Real v = 0; return parse_real(wire_value, v) && set_real(d->id, v, external_write); }
    if (d->kind == Kind::boolean) { bool v = false; return parse_bool(wire_value, v) && set_bool(d->id, v, external_write); }
    return set_string(d->id, strip_quotes(wire_value), external_write);
}

bool ValueRegistry::set_real(std::string_view name, Real value, bool external_write)
{
    const Descriptor* d = find(name);
    return d && set_real(d->id, value, external_write);
}

bool ValueRegistry::set_bool(std::string_view name, bool value, bool external_write)
{
    const Descriptor* d = find(name);
    return d && set_bool(d->id, value, external_write);
}

bool ValueRegistry::set_string(std::string_view name, std::string_view value, bool external_write)
{
    const Descriptor* d = find(name);
    return d && set_string(d->id, value, external_write);
}

bool ValueRegistry::read_real(std::string_view name, Real& out) const
{
    const Descriptor* d = find(name);
    return d && read_real(d->id, out);
}

bool ValueRegistry::read_bool(std::string_view name, bool& out) const
{
    const Descriptor* d = find(name);
    return d && read_bool(d->id, out);
}

bool ValueRegistry::read_string(std::string_view name, std::string_view& out) const
{
    const Descriptor* d = find(name);
    return d && read_string(d->id, out);
}

bool ValueRegistry::set_real(FieldId id, Real value, bool)
{
    if (!state_) return false;
    switch (id) {
    case FieldId::server_port: state_->server.port = static_cast<uint16_t>(value); break;
    case FieldId::ap_heading: state_->ap.heading = value; break;
    case FieldId::ap_heading_command: state_->ap.heading_command = value; break;
    case FieldId::ap_heading_error: state_->ap.heading_error = value; break;
    case FieldId::ap_command: state_->ap.command = value; break;
    case FieldId::rudder_angle: state_->rudder.angle.value = value; state_->rudder.angle.valid = true; break;
    case FieldId::rudder_offset: state_->rudder.offset = value; break;
    case FieldId::rudder_range: state_->rudder.range = value; break;
    case FieldId::servo_command: state_->servo.command = value; break;
    case FieldId::servo_current: state_->servo.current = value; break;
    case FieldId::servo_voltage: state_->servo.voltage = value; break;
    default: return false;
    }
    mark_dirty(id);
    return true;
}

bool ValueRegistry::set_bool(FieldId id, bool value, bool)
{
    if (!state_) return false;
    switch (id) {
    case FieldId::server_running: state_->server.running = value; break;
    case FieldId::ap_enabled: state_->ap.enabled = value; break;
    case FieldId::servo_engaged: state_->servo.engaged = value; break;
    case FieldId::servo_fault: state_->servo.fault = value; break;
    default: return false;
    }
    mark_dirty(id);
    return true;
}

bool ValueRegistry::set_string(FieldId id, std::string_view value, bool)
{
    if (!state_) return false;
    if (id == FieldId::ap_mode) state_->ap.mode = std::string(value);
    else if (id == FieldId::ap_pilot) state_->ap.pilot = std::string(value);
    else return false;
    mark_dirty(id);
    return true;
}

bool ValueRegistry::read_real(FieldId id, Real& out) const
{
    if (!state_) return false;
    switch (id) {
    case FieldId::server_port: out = static_cast<Real>(state_->server.port); return true;
    case FieldId::ap_heading: out = state_->ap.heading; return true;
    case FieldId::ap_heading_command: out = state_->ap.heading_command; return true;
    case FieldId::ap_heading_error: out = state_->ap.heading_error; return true;
    case FieldId::ap_command: out = state_->ap.command; return true;
    case FieldId::rudder_angle: out = state_->rudder.angle.value; return state_->rudder.angle.valid;
    case FieldId::rudder_offset: out = state_->rudder.offset; return true;
    case FieldId::rudder_range: out = state_->rudder.range; return true;
    case FieldId::servo_command: out = state_->servo.command; return true;
    case FieldId::servo_current: out = state_->servo.current; return true;
    case FieldId::servo_voltage: out = state_->servo.voltage; return true;
    default: return false;
    }
}

bool ValueRegistry::read_bool(FieldId id, bool& out) const
{
    if (!state_) return false;
    switch (id) {
    case FieldId::server_running: out = state_->server.running; return true;
    case FieldId::ap_enabled: out = state_->ap.enabled; return true;
    case FieldId::servo_engaged: out = state_->servo.engaged; return true;
    case FieldId::servo_fault: out = state_->servo.fault; return true;
    default: return false;
    }
}

bool ValueRegistry::read_string(FieldId id, std::string_view& out) const
{
    if (!state_) return false;
    if (id == FieldId::ap_mode) { out = state_->ap.mode; return true; }
    if (id == FieldId::ap_pilot) { out = state_->ap.pilot; return true; }
    return false;
}

bool ValueRegistry::write_value(std::string_view name, ValueWriter& out) const
{
    const Descriptor* d = find(name);
    if (!d) return false;
    Real r = 0;
    bool b = false;
    std::string_view s;
    if (d->kind == Kind::real) return read_real(d->id, r) && out.write_real(r);
    if (d->kind == Kind::boolean) return read_bool(d->id, b) && out.write_bool(b);
    return read_string(d->id, s) && out.write_json_string(s);
}

bool ValueRegistry::write_value_line(std::string_view name, ValueWriter& out) const
{
    const Descriptor* d = find(name);
    return d && write_value_line(d->id, out);
}

bool ValueRegistry::write_value_line(FieldId id, ValueWriter& out) const
{
    const Descriptor* d = descriptor(id);
    return d && out.write(d->name) && out.write_char('=') && write_value(d->name, out) && out.write_char('\n');
}

bool ValueRegistry::write_info_json(FieldId id, ValueWriter& out) const
{
    const Descriptor* d = descriptor(id);
    if (!d) return false;
    return out.write("{\"type\":") && out.write_json_string(d->info.type) && (!d->info.writable || out.write(",\"writable\":true")) && out.write_char('}');
}

bool ValueRegistry::write_values_json(ValueWriter& out) const
{
    if (!out.write_char('{')) return false;
    bool first = true;
    for (const auto& d : fields) {
        if (!first && !out.write_char(',')) return false;
        first = false;
        if (!out.write_json_string(d.name) || !out.write_char(':') || !write_info_json(d.id, out)) return false;
    }
    return out.write_char('}');
}

bool ValueRegistry::watch(std::string_view name, Real period_seconds, uint64_t now_us, ValueWriter* initial_line)
{
    const Descriptor* d = find(name);
    if (!d) return false;
    if (initial_line) write_value_line(d->id, *initial_line);
    for (auto& w : watches_) if (w.active && w.id == d->id) { w.period_seconds = period_seconds; w.next_due_us = now_us + period_us(period_seconds); return true; }
    for (auto& w : watches_) if (!w.active) { w.active = true; w.id = d->id; w.period_seconds = period_seconds; w.next_due_us = now_us + period_us(period_seconds); return true; }
    return false;
}

bool ValueRegistry::unwatch(std::string_view name)
{
    const Descriptor* d = find(name);
    if (!d) return false;
    for (auto& w : watches_) if (w.active && w.id == d->id) w.active = false;
    return true;
}

void ValueRegistry::mark_dirty(FieldId id)
{
    const size_t index = static_cast<size_t>(id);
    if (index < dirty_.size()) dirty_[index] = true;
}

void ValueRegistry::emit_changed_watches(ValueWriter& out)
{
    for (size_t i = 0; i < dirty_.size(); ++i) {
        if (!dirty_[i]) continue;
        const FieldId id = static_cast<FieldId>(i);
        bool watched = false;
        for (const auto& w : watches_) if (w.active && w.id == id && w.period_seconds == Real{0}) { watched = true; break; }
        if (watched) write_value_line(id, out);
        dirty_[i] = false;
    }
}

void ValueRegistry::emit_due_watches(uint64_t now_us, ValueWriter& out)
{
    for (auto& w : watches_) {
        if (!w.active || w.period_seconds <= Real{0} || now_us < w.next_due_us) continue;
        write_value_line(w.id, out);
        const uint64_t period = period_us(w.period_seconds);
        do { w.next_due_us += period; } while (period && now_us >= w.next_due_us);
    }
}

} // namespace pypilot
