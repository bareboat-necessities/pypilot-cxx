#pragma once

#include <cstdint>
#include <string>

namespace pypilot::syslib::data_model {

enum class SensorSource {
    none,
    gpsd,
    servo,
    serial,
    tcp,
    signalk,
    water_wind,
    gps_wind,
};

inline int source_priority(SensorSource source)
{
    switch (source) {
    case SensorSource::gpsd: return 1;
    case SensorSource::servo: return 1;
    case SensorSource::serial: return 2;
    case SensorSource::tcp: return 3;
    case SensorSource::signalk: return 4;
    case SensorSource::water_wind: return 5;
    case SensorSource::gps_wind: return 6;
    case SensorSource::none: return 7;
    }
    return 7;
}

template <typename RealT>
struct SourceStamped {
    RealT value{};
    uint64_t timestamp_us = 0;
    SensorSource source = SensorSource::none;
    bool valid = false;
};

template <typename RealT = float>
struct DataModel {
    struct Server { bool running = false; uint16_t port = 23322; } server;
    struct Status { bool enabled = false; bool imu_ready = false; std::string mode = "compass"; std::string pilot = "basic"; } status;
    struct Ap {
        bool enabled = false;
        RealT heading = 0;
        RealT heading_command = 0;
        RealT heading_error = 0;
        RealT command = 0;
        std::string mode = "compass";
        std::string pilot = "basic";
        struct Tack { bool state = false; int direction = 0; RealT timeout = 0; } tack;
    } ap;
    struct Imu { RealT heading = 0; RealT pitch = 0; RealT roll = 0; RealT heading_rate = 0; bool valid = false; uint64_t timestamp_us = 0; } imu;
    struct Gps { SourceStamped<RealT> speed; SourceStamped<RealT> track; SourceStamped<RealT> latitude; SourceStamped<RealT> longitude; } gps;
    struct Wind { SourceStamped<RealT> apparent_direction; SourceStamped<RealT> apparent_speed; SourceStamped<RealT> true_direction; SourceStamped<RealT> true_speed; } wind;
    struct Water { SourceStamped<RealT> speed; SourceStamped<RealT> leeway; } water;
    struct Rudder { SourceStamped<RealT> angle; RealT offset = 0; RealT range = 45; } rudder;
    struct Navigation { struct Apb { SourceStamped<RealT> xte_nmi; SourceStamped<RealT> track_deg; std::string sender_id; bool mode_hint = false; } apb; } navigation;
    struct Servo { RealT command = 0; RealT current = 0; RealT voltage = 0; bool engaged = false; bool fault = false; } servo;
};

} // namespace pypilot::syslib::data_model
