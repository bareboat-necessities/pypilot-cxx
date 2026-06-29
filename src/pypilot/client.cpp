#include "pypilot/pypilot.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace pypilot {
namespace {
std::string_view trim_view(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.remove_suffix(1);
    return s;
}

struct PypilotRecordView {
    std::string_view name;
    std::string_view json;
};

bool parse_record(std::string_view line, PypilotRecordView& out)
{
    line = trim_view(line);
    const size_t eq = line.find('=');
    if (line.empty() || eq == std::string_view::npos || eq == 0) return false;
    out.name = trim_view(line.substr(0, eq));
    out.json = trim_view(line.substr(eq + 1));
    return !out.name.empty();
}

size_t append_raw(char* out, size_t cap, size_t pos, std::string_view s)
{
    if (!out || cap == 0) return pos;
    for (char c : s) {
        if (pos + 1 >= cap) break;
        out[pos++] = c;
    }
    out[pos] = '\0';
    return pos;
}

void trim_number(char* s)
{
    if (!s) return;
    char* first = s;
    while (*first == ' ') ++first;
    if (first != s) std::memmove(s, first, std::strlen(first) + 1);
    char* end = s + std::strlen(s);
    while (end > s && end[-1] == '0') --end;
    if (end > s && end[-1] == '.') --end;
    if (end == s || (end == s + 1 && s[0] == '-')) *end++ = '0';
    *end = '\0';
}

size_t append_real(char* out, size_t cap, size_t pos, Real value)
{
    char number[32];
    std::snprintf(number, sizeof(number), "%.6f", static_cast<double>(value));
    trim_number(number);
    return append_raw(out, cap, pos, number);
}

size_t append_json_string(char* out, size_t cap, size_t pos, std::string_view s)
{
    pos = append_raw(out, cap, pos, "\"");
    for (char c : s) {
        if (c == '"' || c == '\\') pos = append_raw(out, cap, pos, "\\");
        else if (c == '\n') { pos = append_raw(out, cap, pos, "\\n"); continue; }
        else if (c == '\r') { pos = append_raw(out, cap, pos, "\\r"); continue; }
        else if (c == '\t') { pos = append_raw(out, cap, pos, "\\t"); continue; }
        pos = append_raw(out, cap, pos, std::string_view(&c, 1));
    }
    return append_raw(out, cap, pos, "\"");
}

bool make_record_raw(char* out, size_t cap, std::string_view name, std::string_view json)
{
    size_t pos = 0;
    pos = append_raw(out, cap, pos, name);
    pos = append_raw(out, cap, pos, "=");
    pos = append_raw(out, cap, pos, json);
    pos = append_raw(out, cap, pos, "\n");
    return pos > 0 && pos + 1 < cap;
}

bool make_watch(char* out, size_t cap, std::string_view name, std::string_view period_json)
{
    size_t pos = 0;
    pos = append_raw(out, cap, pos, "watch={\"");
    pos = append_raw(out, cap, pos, name);
    pos = append_raw(out, cap, pos, "\":");
    pos = append_raw(out, cap, pos, period_json);
    pos = append_raw(out, cap, pos, "}\n");
    return pos > 0 && pos + 1 < cap;
}

bool make_watch_period(char* out, size_t cap, std::string_view name, Real period_seconds)
{
    if (period_seconds <= Real{0}) return make_watch(out, cap, name, "true");
    char period[32];
    append_real(period, sizeof(period), 0, period_seconds);
    return make_watch(out, cap, name, period);
}
} // namespace

PypilotClient::PypilotClient(EventLoop& loop, PypilotState& state, ValueRegistry& values)
    : loop_(loop), state_(state), values_(values), line_reader_(tcp_stream_)
{
    values_.attach(state_);
    line_reader_.on_data_ready(this, &PypilotClient::on_line_ready);
}

void PypilotClient::close()
{
    if (line_event_) {
        loop_.cancel(line_event_);
        line_event_ = 0;
    }
    if (connection_) connection_->close();
    connection_ = nullptr;
    tcp_stream_.detach();
    line_reader_.reset();
}

void PypilotClient::poll()
{
    loop_.run_once();
}

void PypilotClient::receive(Real timeout_seconds)
{
    poll();
    if (timeout_seconds > Real{0}) loop_.sleep_us(seconds_to_us(timeout_seconds));
}

void PypilotClient::watch(std::string name, Real period_seconds)
{
    watch(std::string_view(name), period_seconds);
}

void PypilotClient::watch(std::string_view name, Real period_seconds)
{
    remember_watch(name, period_seconds);
    write_watch_line(name, period_seconds);
}

void PypilotClient::unwatch(std::string_view name)
{
    for (auto& watch : watches_) {
        if (watch.active && name == watch.name) watch.active = false;
    }
    char line[MaxLineSize];
    if (make_watch(line, sizeof(line), name, "false")) queue_or_write(line);
}

void PypilotClient::set(std::string name, std::string wire_value)
{
    char line[MaxLineSize];
    if (make_record_raw(line, sizeof(line), name, wire_value)) queue_or_write(line);
}

void PypilotClient::set_bool(std::string_view name, bool value)
{
    char line[MaxLineSize];
    if (make_record_raw(line, sizeof(line), name, value ? "true" : "false")) queue_or_write(line);
}

void PypilotClient::set_real(std::string_view name, Real value)
{
    char json[32];
    append_real(json, sizeof(json), 0, value);
    char line[MaxLineSize];
    if (make_record_raw(line, sizeof(line), name, json)) queue_or_write(line);
}

void PypilotClient::set_string(std::string_view name, std::string_view value)
{
    char json[128];
    append_json_string(json, sizeof(json), 0, value);
    char line[MaxLineSize];
    if (make_record_raw(line, sizeof(line), name, json)) queue_or_write(line);
}

bool PypilotClient::send_raw(std::string_view line)
{
    return queue_or_write(line);
}

std::optional<std::string> PypilotClient::pop_outgoing()
{
    for (auto& pending : pending_) {
        if (!pending.active) continue;
        std::string out(pending.bytes, pending.size);
        pending.active = false;
        pending.size = 0;
        pending.bytes[0] = '\0';
        return out;
    }
    return std::nullopt;
}

void PypilotClient::handle_line(std::string_view line)
{
    PypilotRecordView record;
    if (!parse_record(line, record)) return;
    if (record.name == "values") {
        // TODO: parse metadata for UI clients that need dynamic value descriptors.
        return;
    }
    values_.set_from_wire(record.name, record.json, false);
}

void PypilotClient::on_connect(async_event_loop::ITcpConnection& connection, const async_event_loop::TcpPeerInfo& peer)
{
    (void)peer;
    connection_ = &connection;
    tcp_stream_.attach(connection);
    line_reader_.reset();
    if (line_event_) loop_.cancel(line_event_);
    line_event_ = loop_.on_bytes_ready(tcp_stream_, [this]() { poll_line_reader(); });
    flush_pending();
    replay_watches();
}

void PypilotClient::on_data(async_event_loop::ITcpConnection& connection)
{
    (void)connection;
    poll_line_reader();
}

void PypilotClient::on_close(async_event_loop::ITcpConnection& connection)
{
    if (&connection == connection_) {
        if (line_event_) {
            loop_.cancel(line_event_);
            line_event_ = 0;
        }
        connection_ = nullptr;
        tcp_stream_.detach();
        line_reader_.reset();
    }
}

void PypilotClient::on_error(int error_code)
{
    last_error_ = error_code;
    if (line_event_) {
        loop_.cancel(line_event_);
        line_event_ = 0;
    }
    connection_ = nullptr;
    tcp_stream_.detach();
    line_reader_.reset();
}

void PypilotClient::poll_line_reader()
{
    if (tcp_stream_.readable()) line_reader_.poll(loop_.now_us());
}

void PypilotClient::on_line_ready(void* context, async_event_loop::LineView line)
{
    auto* self = static_cast<PypilotClient*>(context);
    if (!self || !line.data) return;
    self->handle_line(std::string_view(line.data, line.size));
}

bool PypilotClient::queue_or_write(std::string_view line)
{
    if (connected()) {
        const int n = connection_->write(reinterpret_cast<const uint8_t*>(line.data()), line.size());
        if (n == static_cast<int>(line.size())) return true;
    }
    return queue_line(line);
}

bool PypilotClient::queue_line(std::string_view line)
{
    if (line.size() >= MaxLineSize) return false;
    for (auto& pending : pending_) {
        if (pending.active) continue;
        pending.active = true;
        pending.size = line.size();
        std::memcpy(pending.bytes, line.data(), line.size());
        pending.bytes[line.size()] = '\0';
        return true;
    }
    return false;
}

void PypilotClient::flush_pending()
{
    if (!connected()) return;
    for (auto& pending : pending_) {
        if (!pending.active) continue;
        const int n = connection_->write(reinterpret_cast<const uint8_t*>(pending.bytes), pending.size);
        if (n == static_cast<int>(pending.size)) {
            pending.active = false;
            pending.size = 0;
            pending.bytes[0] = '\0';
        }
    }
}

void PypilotClient::replay_watches()
{
    for (const auto& watch : watches_) {
        if (watch.active) write_watch_line(watch.name, watch.period_seconds);
    }
}

void PypilotClient::remember_watch(std::string_view name, Real period_seconds)
{
    for (auto& watch : watches_) {
        if (watch.active && name == watch.name) {
            watch.period_seconds = period_seconds;
            return;
        }
    }
    for (auto& watch : watches_) {
        if (!watch.active) {
            watch.active = true;
            copy_name(watch.name, sizeof(watch.name), name);
            watch.period_seconds = period_seconds;
            return;
        }
    }
}

bool PypilotClient::write_watch_line(std::string_view name, Real period_seconds)
{
    char line[MaxLineSize];
    return make_watch_period(line, sizeof(line), name, period_seconds) && queue_or_write(line);
}

bool PypilotClient::copy_name(char* dst, size_t dst_size, std::string_view src)
{
    if (!dst || dst_size == 0) return false;
    const size_t n = src.size() < dst_size - 1 ? src.size() : dst_size - 1;
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
    return n == src.size();
}

} // namespace pypilot
