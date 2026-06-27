#include "pypilot/pypilot.hpp"

#include <cstdlib>

namespace pypilot {
namespace {
std::string trim(std::string_view s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.remove_suffix(1);
    return std::string(s);
}

bool parse_double(std::string_view s, double& v)
{
    const std::string copy = trim(s);
    if (copy.empty()) return false;
    char* end = nullptr;
    v = std::strtod(copy.c_str(), &end);
    return end != copy.c_str();
}

struct WatchItem { std::string name; bool remove = false; double period = 0.0; };

bool parse_watch_object(std::string_view json, std::vector<WatchItem>& out)
{
    std::string s = trim(json);
    if (s.size() < 2 || s.front() != '{' || s.back() != '}') return false;
    s = s.substr(1, s.size() - 2);
    size_t pos = 0;
    while (pos < s.size()) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == ',')) ++pos;
        if (pos >= s.size()) break;
        if (s[pos] != '"') return false;
        const size_t key_end = s.find('"', pos + 1);
        if (key_end == std::string::npos) return false;
        WatchItem item;
        item.name = s.substr(pos + 1, key_end - pos - 1);
        pos = key_end + 1;
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
        if (pos >= s.size() || s[pos] != ':') return false;
        ++pos;
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t')) ++pos;
        const size_t value_start = pos;
        while (pos < s.size() && s[pos] != ',') ++pos;
        const std::string value = trim(std::string_view(s).substr(value_start, pos - value_start));
        if (value == "false") item.remove = true;
        else if (value == "true") item.period = 0.0;
        else if (!parse_double(value, item.period)) return false;
        out.push_back(item);
        if (pos < s.size() && s[pos] == ',') ++pos;
    }
    return true;
}
} // namespace

PypilotServer::PypilotServer(EventLoop& loop, PypilotState& state, ValueRegistry& values)
    : loop_(loop), state_(state), values_(values)
{
    values_.attach(state_);
    state_.server.running = true;
    running_ = true;
    values_.set_bool("server.running", true);
    values_.set_number("server.port", state_.server.port);
}

void PypilotServer::poll()
{
    loop_.run_once();
    for (auto& line : values_.take_changed_watches()) output_.push_back(std::move(line));
    for (auto& line : values_.take_due_watches(loop_.now_us())) output_.push_back(std::move(line));
    // TODO: translate server.py sockets, periodic storage, and datagram output.
}

std::vector<std::string> PypilotServer::handle_line(std::string_view line)
{
    const std::string l = trim(line);
    if (l.empty()) return {};
    const size_t eq = l.find('=');
    if (eq == std::string::npos) return error("invalid request: missing '='");
    const std::string name = trim(std::string_view(l).substr(0, eq));
    const std::string value = trim(std::string_view(l).substr(eq + 1));
    if (name == "watch") return handle_watch(value);
    if (name == "values") return error("value registration not implemented");
    if (name == "udp_port") return {};
    return handle_assignment(name, value);
}

std::vector<std::string> PypilotServer::handle_assignment(std::string_view name, std::string_view value)
{
    if (!values_.set_from_wire(name, value, true)) return error("invalid set: " + std::string(name));
    return {};
}

std::vector<std::string> PypilotServer::handle_watch(std::string_view expression)
{
    std::vector<WatchItem> items;
    if (!parse_watch_object(expression, items)) return error("invalid watch request");
    std::vector<std::string> responses;
    for (const auto& item : items) {
        if (item.name == "values") {
            if (!item.remove) responses.push_back("values=" + values_.values_json() + "\n");
            continue;
        }
        if (item.remove) {
            if (!values_.unwatch(item.name)) responses.push_back("error=cannot remove unknown watch for " + item.name + "\n");
            continue;
        }
        std::string initial;
        if (!values_.watch(item.name, item.period, loop_.now_us(), &initial)) responses.push_back("error=invalid unknown value: " + item.name + "\n");
        else if (!initial.empty()) responses.push_back(initial);
    }
    return responses;
}

std::vector<std::string> PypilotServer::collect_output()
{
    poll();
    std::vector<std::string> out;
    while (!output_.empty()) { out.push_back(std::move(output_.front())); output_.pop_front(); }
    return out;
}

std::vector<std::string> PypilotServer::error(std::string message) const
{
    return {"error=" + message + "\n"};
}

} // namespace pypilot
