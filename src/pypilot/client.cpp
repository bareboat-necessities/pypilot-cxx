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
        pos = append_raw(out, cap, pos, std::string_view(&c, 1));
    }
    return append_raw(out, cap, pos, "\"");
}

bool make_watch_line(char* out, size_t cap, std::string_view name, Real period_seconds, bool unwatch)
{
    size_t pos = 0;
    pos = append_raw(out, cap, pos, "watch={\"");
    pos = append_raw(out, cap, pos, name);
    pos = append_raw(out, cap, pos, "\":");
    if (unwatch) pos = append_raw(out, cap, pos, "false");
    else if (period_seconds <= Real{0}) pos = append_raw(out, cap, pos, "true");
    else pos = append_real(out, cap, pos, period_seconds);
    pos = append_raw(out, cap, pos, "}\n");
    return pos > 0 && pos + 1 < cap;
}

bool make_set_line(char* out, size_t cap, std::string_view name, std::string_view wire_value)
{
    size_t pos = 0;
    pos = append_raw(out, cap, pos, name);
    pos = append_raw(out, cap, pos, "=");
    pos = append_raw(out, cap, pos, wire_value);
    pos = append_raw(out, cap, pos, "\n");
    return pos > 0 && pos + 1 < cap;
}
} // namespace

PypilotClient::PypilotClient(EventLoop& loop, PypilotState& state, ValueRegistry& values)
    : loop_(loop), state_(state), values_(values), tcp_client_(loop)
{
    values_.attach(state_);
}

bool PypilotClient::connect(const char* host, uint16_t port)
{
    copy_name(host_, sizeof(host_), host ? std::string_view(host) : std::string_view("127.0.0.1"));
    port_ = port;
    syslib::event_loop::TcpConnectOptions options;
    options.host = host_;
    options.port = port_;
    return tcp_client_.connect(options, *this);
}

bool PypilotClient::connected() const
{
    return connection_ && connection_->valid();
}

void PypilotClient::close()
{
    if (connection_) connection_->close();
    connection_ = nullptr;
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
    for (auto& watch : watches_) if (watch.active && name == watch.name) watch.active = false;
    char line[MaxLineSize];
    if (make_watch_line(line, sizeof(line), name, Real{0}, true)) queue_or_write(line);
}

void PypilotClient::set(std::string name, std::string wire_value)
{
    char line[MaxLineSize];
    if (make_set_line(line, sizeof(line), name, wire_value)) queue_or_write(line);
}

void PypilotClient::set_bool(std::string_view name, bool value)
{
    char line[MaxLineSize];
    if (make_set_line(line, sizeof(line), name, value ? "true" : "false")) queue_or_write(line);
}

void PypilotClient::set_real(std::string_view name, Real value)
{
    char value_buf[32]{};
    append_real(value_buf, sizeof(value_buf), 0, value);
    char line[MaxLineSize];
    if (make_set_line(line, sizeof(line), name, value_buf)) queue_or_write(line);
}

void PypilotClient::set_string(std::string_view name, std::string_view value)
{
    char value_buf[128]{};
    append_json_string(value_buf, sizeof(value_buf), 0, value);
    char line[MaxLineSize];
    if (make_set_line(line, sizeof(line), name, value_buf)) queue_or_write(line);
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
    line = trim_view(line);
    if (line.empty()) return;
    const size_t eq = line.find('=');
    if (eq == std::string_view::npos || eq == 0) return;
    const std::string_view name = trim_view(line.substr(0, eq));
    const std::string_view value = trim_view(line.substr(eq + 1));
    if (name == "values") {
        // TODO: parse values metadata if the UI/client side needs dynamic descriptors.
        return;
    }
    values_.set_from_wire(name, value, false);
}

void PypilotClient::on_connect(syslib::event_loop::ITcpConnection& connection, const syslib::event_loop::TcpPeerInfo& peer)
{
    (void)peer;
    connection_ = &connection;
    flush_pending();
    replay_watches();
}

void PypilotClient::on_data(syslib::event_loop::ITcpConnection& connection)
{
    char line[MaxLineSize];
    while (connection.read_line(line, sizeof(line))) {
        handle_line(line);
    }
}

void PypilotClient::on_close(syslib::event_loop::ITcpConnection& connection)
{
    if (&connection == connection_) connection_ = nullptr;
}

void PypilotClient::on_error(int error_code)
{
    last_error_ = error_code;
    connection_ = nullptr;
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
    return make_watch_line(line, sizeof(line), name, period_seconds, false) && queue_or_write(line);
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
