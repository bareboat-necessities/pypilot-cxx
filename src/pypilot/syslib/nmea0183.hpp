#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "pypilot/syslib/data_model.hpp"

namespace pypilot::syslib::nmea0183 {
using Real = data_model::Real;

struct Sentence { std::string talker; std::string type; std::vector<std::string> fields; bool checksum_ok = false; };

inline uint8_t checksum_body(std::string_view body) { uint8_t checksum = 0; for (char c : body) checksum ^= static_cast<uint8_t>(c); return checksum; }
inline int hex_value(char c) { if (c >= '0' && c <= '9') return c - '0'; if (c >= 'A' && c <= 'F') return c - 'A' + 10; if (c >= 'a' && c <= 'f') return c - 'a' + 10; return -1; }

inline bool parse_real(std::string_view field, Real& value)
{
    if (field.empty()) return false;
    std::string copy(field);
    char* end = nullptr;
    value = std::strtof(copy.c_str(), &end);
    return end != copy.c_str();
}

inline std::vector<std::string> split_fields(std::string_view body)
{
    std::vector<std::string> fields;
    size_t start = 0;
    while (start <= body.size()) {
        const size_t comma = body.find(',', start);
        if (comma == std::string_view::npos) { fields.emplace_back(body.substr(start)); break; }
        fields.emplace_back(body.substr(start, comma - start));
        start = comma + 1;
    }
    return fields;
}

inline bool parse_line(std::string_view line, Sentence& sentence)
{
    sentence = Sentence{};
    if (line.empty() || line.front() != '$') return false;
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.remove_suffix(1);
    const size_t star = line.find('*');
    std::string_view body = line.substr(1, star == std::string_view::npos ? std::string_view::npos : star - 1);
    if (star != std::string_view::npos && star + 2 < line.size()) {
        const int hi = hex_value(line[star + 1]);
        const int lo = hex_value(line[star + 2]);
        sentence.checksum_ok = hi >= 0 && lo >= 0 && checksum_body(body) == static_cast<uint8_t>((hi << 4) | lo);
    }
    auto parts = split_fields(body);
    if (parts.empty() || parts[0].size() < 3) return false;
    const std::string& header = parts[0];
    sentence.talker = header.substr(0, header.size() > 3 ? header.size() - 3 : 0);
    sentence.type = header.substr(header.size() - 3);
    sentence.fields.assign(parts.begin() + 1, parts.end());
    return true;
}

template <typename RealT>
inline bool apply_sentence(const Sentence& sentence, data_model::DataModel<RealT>& model, uint64_t now_us, data_model::SensorSource source)
{
    if (sentence.type == "HDT" || sentence.type == "HDM") {
        if (!sentence.fields.empty()) {
            Real heading = 0;
            if (parse_real(sentence.fields[0], heading)) { model.imu.heading = static_cast<RealT>(heading); model.imu.valid = true; model.imu.timestamp_us = now_us; return true; }
        }
    }
    if (sentence.type == "MWV") {
        if (sentence.fields.size() >= 4) {
            Real angle = 0; Real speed = 0;
            if (parse_real(sentence.fields[0], angle) && parse_real(sentence.fields[2], speed)) {
                model.wind.apparent_direction.value = static_cast<RealT>(angle); model.wind.apparent_direction.timestamp_us = now_us; model.wind.apparent_direction.source = source; model.wind.apparent_direction.valid = true;
                model.wind.apparent_speed.value = static_cast<RealT>(speed); model.wind.apparent_speed.timestamp_us = now_us; model.wind.apparent_speed.source = source; model.wind.apparent_speed.valid = true;
                return true;
            }
        }
    }
    if (sentence.type == "RSA") {
        if (!sentence.fields.empty()) {
            Real angle = 0;
            if (parse_real(sentence.fields[0], angle)) { model.rudder.angle.value = static_cast<RealT>(-angle); model.rudder.angle.timestamp_us = now_us; model.rudder.angle.source = source; model.rudder.angle.valid = true; return true; }
        }
    }
    if (sentence.type == "APB") { model.navigation.apb.sender_id = sentence.talker; model.navigation.apb.mode_hint = true; return true; }
    return false;
}

} // namespace pypilot::syslib::nmea0183
