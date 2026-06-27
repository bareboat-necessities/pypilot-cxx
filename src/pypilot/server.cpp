#include "pypilot/pypilot.hpp"

#include <cstdlib>
#include <sstream>

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
}

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
    // TODO: translate server.py TCP accept/read/write behavior using syslib streams.
    // TODO: add UDP watch output and persistent profile storage.
}

std::vector<std::string> PypilotServer::handle_line(std::string_view line)
{
    const std::string l = trim(line);
    if (l.empty()) return {};

    if (l == "list" || l == "values") return {"values=" + values_.values_json() + "\n"};

    if (l.rfind("get ", 0) == 0) {
        const std::string name = trim(std::string_view(l).substr(4));
        auto wire = values_.get_wire(name);
        if (!wire) return error("unknown value: " + name);
        return {name + "=" + *wire + "\n"};
    }

    if (l.rfind("watch ", 0) == 0) return handle_watch(std::string_view(l).substr(6));

    const size_t eq = l.find('=');
    if (eq != std::string::npos) {
        const std::string name = trim(std::string_view(l).substr(0, eq));
        const std::string value = trim(std::string_view(l).substr(eq + 1));
        if (name == "watch") return handle_watch(value);
        return handle_assignment(name, value);
    }

    auto wire = values_.get_wire(l);
    if (!wire) return error("unknown value: " + l);
    return {l + "=" + *wire + "\n"};
}

std::vector<std::string> PypilotServer::handle_assignment(std::string_view name, std::string_view value)
{
    if (!values_.set_from_wire(name, value, true)) return error("invalid set: " + std::string(name));
    auto wire = values_.get_wire(name);
    if (!wire) return {};
    return {std::string(name) + "=" + *wire + "\n"};
}

std::vector<std::string> PypilotServer::handle_watch(std::string_view expression)
{
    const std::string expr = trim(expression);
    const size_t eq = expr.find('=');
    std::string name;
    double period = 0.0;
    if (eq == std::string::npos) {
        name = trim(expr);
    } else {
        name = trim(std::string_view(expr).substr(0, eq));
        const std::string rhs = trim(std::string_view(expr).substr(eq + 1));
        if (rhs == "false") {
            if (!values_.unwatch(name)) return error("unknown watch: " + name);
            return {"watch=" + name + ":false\n"};
        }
        if (rhs == "true") {
            period = 0.0;
        } else if (!parse_double(rhs, period)) {
            return error("invalid watch period: " + rhs);
        }
    }

    std::string initial;
    if (!values_.watch(name, period, loop_.now_us(), &initial)) return error("unknown watch: " + name);
    if (!initial.empty()) return {initial};
    return {};
}

std::vector<std::string> PypilotServer::collect_output()
{
    poll();
    std::vector<std::string> out;
    while (!output_.empty()) {
        out.push_back(std::move(output_.front()));
        output_.pop_front();
    }
    return out;
}

std::vector<std::string> PypilotServer::error(std::string message) const
{
    return {"error=" + message + "\n"};
}

} // namespace pypilot
