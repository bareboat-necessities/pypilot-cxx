#include "pypilot/pypilot.hpp"

#include <cstdlib>

namespace pypilot {
namespace {
std::string_view trim_view(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.remove_suffix(1);
    return s;
}

bool parse_real(std::string_view s, Real& out)
{
    s = trim_view(s);
    char buf[32]{};
    const size_t n = s.size() < sizeof(buf) - 1 ? s.size() : sizeof(buf) - 1;
    for (size_t i = 0; i < n; ++i) buf[i] = s[i];
    char* end = nullptr;
    out = std::strtof(buf, &end);
    return end != buf;
}
} // namespace

PypilotServer::PypilotServer(EventLoop& loop, PypilotState& state, ValueRegistry& values)
    : loop_(loop), state_(state), values_(values)
{
    values_.attach(state_);
    state_.server.running = true;
    running_ = true;
    values_.set_bool("server.running", true);
    values_.set_real("server.port", static_cast<Real>(state_.server.port));
}

void PypilotServer::poll()
{
    loop_.run_once();
}

void PypilotServer::poll(ValueWriter& out)
{
    poll();
    values_.emit_changed_watches(out);
    values_.emit_due_watches(loop_.now_us(), out);
}

bool PypilotServer::handle_line(std::string_view line, ValueWriter& out)
{
    line = trim_view(line);
    if (line.empty()) return true;
    const size_t eq = line.find('=');
    if (eq == std::string_view::npos) return error(out, "invalid request: missing '='");
    const std::string_view name = trim_view(line.substr(0, eq));
    const std::string_view value = trim_view(line.substr(eq + 1));
    if (name == "watch") return handle_watch(value, out);
    if (name == "values") return error(out, "remote value registration not implemented");
    if (name == "udp_port") return true;
    return handle_assignment(name, value, out);
}

std::string PypilotServer::handle_line(std::string_view line)
{
    std::string out;
    StringValueWriter writer(out);
    handle_line(line, writer);
    return out;
}

std::string PypilotServer::collect_output()
{
    std::string out;
    StringValueWriter writer(out);
    poll(writer);
    return out;
}

bool PypilotServer::handle_assignment(std::string_view name, std::string_view value, ValueWriter& out)
{
    if (!values_.set_from_wire(name, value, true)) return error(out, "invalid set");
    return true;
}

bool PypilotServer::handle_watch(std::string_view expression, ValueWriter& out)
{
    expression = trim_view(expression);
    if (expression.size() < 2 || expression.front() != '{' || expression.back() != '}') return error(out, "invalid watch request");
    expression.remove_prefix(1);
    expression.remove_suffix(1);

    while (!expression.empty()) {
        expression = trim_view(expression);
        if (!expression.empty() && expression.front() == ',') { expression.remove_prefix(1); continue; }
        expression = trim_view(expression);
        if (expression.empty()) break;
        if (expression.front() != '"') return error(out, "invalid watch key");
        expression.remove_prefix(1);
        const size_t key_end = expression.find('"');
        if (key_end == std::string_view::npos) return error(out, "invalid watch key");
        const std::string_view key = expression.substr(0, key_end);
        expression.remove_prefix(key_end + 1);
        expression = trim_view(expression);
        if (expression.empty() || expression.front() != ':') return error(out, "invalid watch separator");
        expression.remove_prefix(1);
        expression = trim_view(expression);
        const size_t comma = expression.find(',');
        std::string_view value = comma == std::string_view::npos ? expression : expression.substr(0, comma);
        expression = comma == std::string_view::npos ? std::string_view{} : expression.substr(comma + 1);
        value = trim_view(value);

        if (key == "values") {
            if (value != "false") {
                if (!out.write("values=") || !values_.write_values_json(out) || !out.write_char('\n')) return false;
            }
            continue;
        }
        if (value == "false") {
            values_.unwatch(key);
            continue;
        }
        Real period = 0;
        if (value != "true" && !parse_real(value, period)) return error(out, "invalid watch period");
        if (!values_.watch(key, period, loop_.now_us(), &out)) return error(out, "invalid unknown value");
    }
    return true;
}

bool PypilotServer::error(ValueWriter& out, std::string_view message) const
{
    return out.write("error=") && out.write(message) && out.write_char('\n');
}

} // namespace pypilot
