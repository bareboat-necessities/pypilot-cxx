#include "pypilot/pypilot.hpp"

#include <algorithm>

namespace pypilot {

bool ValueRegistry::set_number(std::string name, double value)
{
    auto& entry = values_[std::move(name)];
    entry.kind = Value::Kind::number;
    entry.number = value;
    return true;
}

bool ValueRegistry::set_bool(std::string name, bool value)
{
    auto& entry = values_[std::move(name)];
    entry.kind = Value::Kind::boolean;
    entry.boolean = value;
    return true;
}

bool ValueRegistry::set_string(std::string name, std::string value)
{
    auto& entry = values_[std::move(name)];
    entry.kind = Value::Kind::string;
    entry.text = std::move(value);
    return true;
}

std::optional<double> ValueRegistry::get_number(std::string_view name) const
{
    const auto it = values_.find(std::string(name));
    if (it == values_.end() || it->second.kind != Value::Kind::number) return std::nullopt;
    return it->second.number;
}

std::optional<bool> ValueRegistry::get_bool(std::string_view name) const
{
    const auto it = values_.find(std::string(name));
    if (it == values_.end() || it->second.kind != Value::Kind::boolean) return std::nullopt;
    return it->second.boolean;
}

std::optional<std::string> ValueRegistry::get_string(std::string_view name) const
{
    const auto it = values_.find(std::string(name));
    if (it == values_.end() || it->second.kind != Value::Kind::string) return std::nullopt;
    return it->second.text;
}

std::vector<std::string> ValueRegistry::names() const
{
    std::vector<std::string> out;
    out.reserve(values_.size());
    for (const auto& item : values_) out.push_back(item.first);
    std::sort(out.begin(), out.end());
    return out;
}

// TODO: translate Value, Property, RangeProperty, EnumProperty, BooleanProperty,
// SensorValue, HeadingOffset, TimeValue, JSON serialization, persistence, and
// watch-trigger rules from original values.py.
// TODO: bridge typed PypilotState fields to PyPilot compatibility names.

} // namespace pypilot
