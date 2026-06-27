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

inline const char* source_name(SensorSource source)
{
    switch (source) {
    case SensorSource::gpsd: return "gpsd";
    case SensorSource::servo: return "servo";
    case SensorSource::serial: return "serial";
    case SensorSource::tcp: return "tcp";
    case SensorSource::signalk: return "signalk";
    case SensorSource::water_wind: return "water+wind";
    case SensorSource::gps_wind: return "gps+wind";
    case SensorSource::none: return "none";
    }
    return "none";
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
    struct Server {
        bool running = false;
        uint16_t port = 23322;
    } server;

    struct Status {
        bool enabled = false;
        bool imu_ready = false;
        std::string mode = "compass";
        std::string pilot = "basic";
    } status;

    struct Ap {
        bool enabled = false;
        RealT heading = 0;
        RealT heading_command = 0;
        RealT heading_error = 0;
        RealT command = 0;
        std::string mode = "compass";
        std::string pilot = "basic";
        struct Tack {
            bool state = false;
            int direction = 0;
            RealT timeout = 0;
        } tack;
    } ap;

    struct Imu {
        RealT heading = 0;
        RealT pitch = 0;
        RealT roll = 0;
        RealT heading_rate = 0;
        bool valid = false;
        uint64_t timestamp_us = 0;
    } imu;

    struct Gps {
        SourceStamped<RealT> speed;
        SourceStamped<RealT> track;
        SourceStamped<RealT> latitude;
        SourceStamped<RealT> longitude;
    } gps;

    struct Wind {
        SourceStamped<RealT> apparent_direction;
        SourceStamped<RealT> apparent_speed;
        SourceStamped<RealT> true_direction;
        SourceStamped<RealT> true_speed;
    } wind;

    struct Water {
        SourceStamped<RealT> speed;
        SourceStamped<RealT> leeway;
    } water;

    struct Rudder {
        SourceStamped<RealT> angle;
        SourceStamped<RealT> speed;
        RealT offset = 0;
        RealT scale = 100;
        RealT nonlinearity = 0;
        RealT range = 45;
        RealT raw = 0;
        bool raw_valid = false;
        uint64_t raw_timestamp_us = 0;
        uint64_t last_angle_time_us = 0;
        RealT last_angle = 0;
        RealT min_raw = static_cast<RealT>(-0.45);
        RealT max_raw = static_cast<RealT>(0.45);
        std::string calibration_state = "idle";
        std::string last_device;
    } rudder;

    struct Navigation {
        struct Apb {
            SourceStamped<RealT> xte_nmi;
            SourceStamped<RealT> track_deg;
            std::string sender_id;
            bool mode_hint = false;
        } apb;
    } navigation;

    struct Servo {
        RealT position_command = 0;
        RealT command = 0;
        RealT duty = 0;
        uint32_t faults = 0;
        RealT voltage = 0;
        RealT current = 0;
        RealT current_noise = 0;
        RealT controller_temp = 0;
        RealT motor_temp = 0;
        RealT max_current = static_cast<RealT>(4.5);
        RealT current_factor = 1;
        RealT current_offset = 0;
        RealT voltage_factor = 1;
        RealT voltage_offset = 0;
        RealT max_controller_temp = 60;
        RealT max_motor_temp = 60;
        RealT max_slew_speed = 28;
        RealT max_slew_slow = 34;
        RealT speed_gain = 0;
        RealT gain = 1;
        RealT clutch_pwm = 100;
        bool use_brake = true;
        bool brake_on = false;
        RealT period = static_cast<RealT>(0.4);
        bool compensate_current = false;
        bool compensate_voltage = false;
        RealT amp_hours = 0;
        RealT watts = 0;
        RealT speed = 0;
        RealT speed_min = 100;
        RealT speed_max = 100;
        RealT position = 0;
        RealT position_p = static_cast<RealT>(0.1);
        RealT position_i = 0;
        RealT position_d = static_cast<RealT>(0.01);
        RealT raw_command = 0;
        bool use_eeprom = true;
        std::string state = "none";
        std::string controller = "none";
        uint32_t flags = 0;
        bool engaged = false;
        bool fault = false;
        uint64_t last_current_time_us = 0;
    } servo;
};

} // namespace pypilot::syslib::data_model
