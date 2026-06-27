#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace pypilot::syslib::servo_protocol {

enum CommandCode : uint8_t {
    COMMAND_CODE = 0xc7,
    RESET_CODE = 0xe7,
    MAX_CURRENT_CODE = 0x1e,
    MAX_CONTROLLER_TEMP_CODE = 0xa4,
    MAX_MOTOR_TEMP_CODE = 0x5a,
    RUDDER_RANGE_CODE = 0xb6,
    RUDDER_MIN_CODE = 0x2b,
    RUDDER_MAX_CODE = 0x4d,
    REPROGRAM_CODE = 0x19,
    DISENGAGE_CODE = 0x68,
    MAX_SLEW_CODE = 0x71,
    EEPROM_READ_CODE = 0x91,
    EEPROM_WRITE_CODE = 0x53,
    CLUTCH_PWM_AND_BRAKE_CODE = 0x36,
};

enum ResultCode : uint8_t {
    CURRENT_CODE = 0x1c,
    VOLTAGE_CODE = 0xb3,
    CONTROLLER_TEMP_CODE = 0xf9,
    MOTOR_TEMP_CODE = 0x48,
    RUDDER_SENSE_CODE = 0xa7,
    FLAGS_CODE = 0x8f,
    EEPROM_VALUE_CODE = 0x9a,
};

enum Telemetry : uint16_t {
    FLAGS = 1,
    CURRENT = 2,
    VOLTAGE = 4,
    SPEED = 8,
    POSITION = 16,
    CONTROLLER_TEMP = 32,
    MOTOR_TEMP = 64,
    RUDDER = 128,
    EEPROM = 256,
};

enum ServoFlags : uint32_t {
    SYNC = 1,
    OVERTEMP_FAULT = 2,
    OVERCURRENT_FAULT = 4,
    ENGAGED = 8,
    INVALID = 16 * 1,
    PORT_PIN_FAULT = 16 * 2,
    STARBOARD_PIN_FAULT = 16 * 4,
    BADVOLTAGE_FAULT = 16 * 8,
    MIN_RUDDER_FAULT = 256 * 1,
    MAX_RUDDER_FAULT = 256 * 2,
    CURRENT_RANGE = 256 * 4,
    BAD_FUSES = 256 * 8,
    REBOOTED = 256 * 16 * 8,
    DRIVER_MASK = 65535,
    PORT_OVERCURRENT_FAULT = 65536 * 1,
    STARBOARD_OVERCURRENT_FAULT = 65536 * 2,
    DRIVER_TIMEOUT = 65536 * 4,
    SATURATED = 65536 * 8,
};

struct Packet {
    uint8_t code = 0;
    uint16_t value = 0;
};

struct TelemetrySample {
    uint16_t mask = 0;
    double current = 0.0;
    double voltage = 0.0;
    double controller_temp = 0.0;
    double motor_temp = 0.0;
    double rudder = std::numeric_limits<double>::quiet_NaN();
    uint32_t flags = 0;
};

struct DriverParams {
    double raw_max_current = 4.5;
    double rudder_min = -0.5;
    double rudder_max = 0.5;
    double max_current = 4.5;
    double max_controller_temp = 60.0;
    double max_motor_temp = 60.0;
    double rudder_range = 45.0;
    double rudder_offset = 0.0;
    double rudder_scale = 100.0;
    double rudder_nonlinearity = 0.0;
    double max_slew_speed = 28.0;
    double max_slew_slow = 34.0;
    double current_factor = 1.0;
    double current_offset = 0.0;
    double voltage_factor = 1.0;
    double voltage_offset = 0.0;
    double min_speed = 100.0;
    double max_speed = 100.0;
    double gain = 1.0;
    double clutch_pwm = 100.0;
    bool brake = true;
};

inline uint8_t crc8_update(uint8_t crc, uint8_t byte)
{
    crc ^= byte;
    for (int i = 0; i < 8; ++i) {
        crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x31) : static_cast<uint8_t>(crc << 1);
    }
    return crc;
}

inline uint8_t crc8(const uint8_t* data, size_t len)
{
    uint8_t crc = 0xff;
    for (size_t i = 0; i < len; ++i) crc = crc8_update(crc, data[i]);
    return crc;
}

inline uint16_t clamp_u16(double value, double lo, double hi)
{
    value = std::clamp(value, lo, hi);
    return static_cast<uint16_t>(std::lround(value));
}

inline std::array<uint8_t, 4> encode(uint8_t code, uint16_t value)
{
    std::array<uint8_t, 4> out{code, static_cast<uint8_t>(value & 0xff), static_cast<uint8_t>((value >> 8) & 0xff), 0};
    out[3] = crc8(out.data(), 3);
    return out;
}

inline bool decode(const uint8_t* bytes, Packet& packet)
{
    if (crc8(bytes, 3) != bytes[3]) return false;
    packet.code = bytes[0];
    packet.value = static_cast<uint16_t>(bytes[1] | (bytes[2] << 8));
    return true;
}

inline std::array<uint8_t, 4> encode_command(double command)
{
    command = std::clamp(command, -1.0, 1.0);
    return encode(COMMAND_CODE, static_cast<uint16_t>(std::lround((command + 1.0) * 1000.0)));
}

inline std::array<uint8_t, 4> encode_reset() { return encode(RESET_CODE, 0); }
inline std::array<uint8_t, 4> encode_disengage() { return encode(DISENGAGE_CODE, 0); }
inline std::array<uint8_t, 4> encode_reprogram() { return encode(REPROGRAM_CODE, 0); }

inline std::optional<std::array<uint8_t, 4>> encode_param_packet(const DriverParams& p, int slot)
{
    switch (slot) {
    case 0: case 8: case 16:
        return encode(MAX_CURRENT_CODE, clamp_u16(p.raw_max_current * 100.0, 0, 6000));
    case 4:
        return encode(MAX_CONTROLLER_TEMP_CODE, clamp_u16(p.max_controller_temp * 100.0, 3000, 10000));
    case 6:
        return encode(MAX_MOTOR_TEMP_CODE, clamp_u16(p.max_motor_temp * 100.0, 3000, 10000));
    case 10:
        return encode(CLUTCH_PWM_AND_BRAKE_CODE, static_cast<uint16_t>(clamp_u16(p.clutch_pwm, 10, 100) | (p.brake ? 256 : 0)));
    case 12:
        return encode(RUDDER_MIN_CODE, clamp_u16((std::clamp(p.rudder_min, -0.5, 0.5) + 0.5) * 65472.0, 0, 65472));
    case 14:
        return encode(RUDDER_MAX_CODE, clamp_u16((std::clamp(p.rudder_max, -0.5, 0.5) + 0.5) * 65472.0, 0, 65472));
    case 18:
        return encode(MAX_SLEW_CODE, static_cast<uint16_t>((clamp_u16(p.max_slew_slow, 0, 100) << 8) | clamp_u16(p.max_slew_speed, 0, 100)));
    default:
        return std::nullopt;
    }
}

inline uint16_t apply_packet(const Packet& packet, TelemetrySample& sample)
{
    switch (packet.code) {
    case CURRENT_CODE:
        sample.current = packet.value / 100.0;
        sample.mask |= CURRENT;
        return CURRENT;
    case VOLTAGE_CODE:
        sample.voltage = packet.value / 100.0;
        sample.mask |= VOLTAGE;
        return VOLTAGE;
    case CONTROLLER_TEMP_CODE:
        sample.controller_temp = static_cast<int16_t>(packet.value) / 100.0;
        sample.mask |= CONTROLLER_TEMP;
        return CONTROLLER_TEMP;
    case MOTOR_TEMP_CODE:
        sample.motor_temp = static_cast<int16_t>(packet.value) / 100.0;
        sample.mask |= MOTOR_TEMP;
        return MOTOR_TEMP;
    case RUDDER_SENSE_CODE:
        sample.rudder = packet.value == 65535 ? std::numeric_limits<double>::quiet_NaN() : packet.value / 65472.0 - 0.5;
        sample.mask |= RUDDER;
        return RUDDER;
    case FLAGS_CODE:
        sample.flags = packet.value;
        sample.mask |= FLAGS;
        return FLAGS;
    case EEPROM_VALUE_CODE:
        sample.mask |= EEPROM;
        return EEPROM;
    default:
        return 0;
    }
}

class Parser {
public:
    uint16_t feed(const uint8_t* bytes, size_t len, TelemetrySample& sample)
    {
        uint16_t changed = 0;
        for (size_t i = 0; i < len; ++i) buffer_.push_back(bytes[i]);
        while (buffer_.size() >= 4) {
            Packet packet;
            if (decode(buffer_.data(), packet)) {
                if (in_sync_count_ >= 2) changed |= apply_packet(packet, sample);
                else ++in_sync_count_;
                buffer_.erase(buffer_.begin(), buffer_.begin() + 4);
            } else {
                in_sync_count_ = 0;
                buffer_.erase(buffer_.begin());
            }
        }
        return changed;
    }

private:
    std::vector<uint8_t> buffer_;
    int in_sync_count_ = 0;
};

} // namespace pypilot::syslib::servo_protocol
