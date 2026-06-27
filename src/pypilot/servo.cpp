#include "pypilot/pypilot.hpp"

#include <algorithm>
#include <cmath>

namespace pypilot {
namespace sp = syslib::servo_protocol;

namespace {
void set_flag(uint32_t& flags, uint32_t flag, bool on)
{
    if (on) flags |= flag;
    else flags &= ~flag;
}

sp::DriverParams params_from_state(const PypilotState& state, int current_multiplier)
{
    sp::DriverParams p;
    p.raw_max_current = current_multiplier * std::max(0.0f, state.servo.max_current - state.servo.current_offset) / std::max(0.001f, state.servo.current_factor);
    p.rudder_min = state.rudder.min_raw;
    p.rudder_max = state.rudder.max_raw;
    p.max_current = state.servo.max_current;
    p.max_controller_temp = state.servo.max_controller_temp;
    p.max_motor_temp = state.servo.max_motor_temp;
    p.rudder_range = state.rudder.range;
    p.rudder_offset = state.rudder.offset;
    p.rudder_scale = state.rudder.scale;
    p.rudder_nonlinearity = state.rudder.nonlinearity;
    p.max_slew_speed = state.servo.max_slew_speed;
    p.max_slew_slow = state.servo.max_slew_slow;
    p.current_factor = state.servo.current_factor;
    p.current_offset = state.servo.current_offset;
    p.voltage_factor = state.servo.voltage_factor;
    p.voltage_offset = state.servo.voltage_offset;
    p.min_speed = state.servo.speed_min;
    p.max_speed = state.servo.speed_max;
    p.gain = state.servo.gain;
    p.clutch_pwm = state.servo.clutch_pwm;
    p.brake = state.servo.brake_on;
    return p;
}
}

Servo::Servo(EventLoop& loop, PypilotState& state) : loop_(loop), state_(state)
{
    command_timeout_us_ = loop_.now_us();
    last_zero_command_us_ = command_timeout_us_;
    raw_command(0);
}

void Servo::queue_packet(const std::array<uint8_t, 4>& packet)
{
    tx_.insert(tx_.end(), packet.begin(), packet.end());
}

std::vector<uint8_t> Servo::take_driver_tx()
{
    std::vector<uint8_t> out;
    out.swap(tx_);
    return out;
}

void Servo::send_driver_params(int current_multiplier)
{
    const auto params = params_from_state(state_, current_multiplier);
    if (auto packet = sp::encode_param_packet(params, out_sync_)) queue_packet(*packet);
    if (++out_sync_ == 23) out_sync_ = 0;
}

void Servo::command(double value)
{
    state_.servo.command = static_cast<Real>(std::clamp(value, -1.0, 1.0));
}

void Servo::raw_command(double value)
{
    value = std::clamp(value, -1.0, 1.0);
    state_.servo.raw_command = static_cast<Real>(value);
    state_.servo.duty = static_cast<Real>(0.001 * (value != 0.0 ? 1.0 : 0.0) + 0.999 * state_.servo.duty);

    const uint64_t now = loop_.now_us();
    if (value == 0.0) {
        state_.servo.speed = 0;
        if (state_.servo.use_brake && state_.servo.brake_on) state_.servo.state = "brake";
        else state_.servo.state = "idle";
        if (now <= command_timeout_us_ + 1000000ULL || now - last_zero_command_us_ >= 200000ULL) {
            queue_packet(sp::encode_command(0));
            last_zero_command_us_ = now;
        }
    } else {
        command_timeout_us_ = now;
        if (value > 0) {
            state_.servo.state = "port";
            lastdir_ = 1;
        } else {
            state_.servo.state = "starboard";
            lastdir_ = -1;
        }
        queue_packet(sp::encode_command(value));
    }
}

void Servo::stop()
{
    state_.servo.brake_on = false;
    raw_command(0);
    lastdir_ = 0;
    state_.servo.state = "stop";
}

void Servo::reset()
{
    queue_packet(sp::encode_reset());
}

void Servo::disengage()
{
    send_driver_params();
    queue_packet(sp::encode_disengage());
    state_.servo.engaged = false;
}

void Servo::set_autopilot_enabled(bool enabled)
{
    ap_enabled_ = enabled;
    state_.servo.engaged = enabled && !disengaged_;
}

bool Servo::fault() const
{
    return (state_.servo.flags & sp::OVERCURRENT_FAULT) != 0 || state_.servo.fault;
}

uint16_t Servo::feed_driver_bytes(const uint8_t* bytes, size_t len)
{
    const uint16_t mask = parser_.feed(bytes, len, sample_);
    if (mask) apply_driver_sample(mask);
    return mask;
}

void Servo::apply_driver_sample(uint16_t mask)
{
    const uint64_t now = loop_.now_us();
    if (mask & sp::VOLTAGE) {
        state_.servo.voltage = static_cast<Real>(state_.servo.voltage_factor * sample_.voltage + state_.servo.voltage_offset);
    }
    if (mask & sp::CURRENT) {
        double current = sample_.current;
        if (current < state_.servo.current_noise * 1.2) current = 0;
        double corrected = state_.servo.current_factor * current;
        if (current) corrected = std::max(0.0, corrected + static_cast<double>(state_.servo.current_offset));
        state_.servo.current = static_cast<Real>(corrected);
        if (state_.servo.last_current_time_us) {
            const double dt = (now - state_.servo.last_current_time_us) * 1.0e-6;
            if (dt > 0.01 && dt < 0.5) {
                if (state_.servo.current) state_.servo.amp_hours += static_cast<Real>(state_.servo.current * dt / 3600.0);
                const double lp = 0.003 * dt;
                state_.servo.watts = static_cast<Real>((1.0 - lp) * state_.servo.watts + lp * state_.servo.voltage * state_.servo.current);
            }
        }
        state_.servo.last_current_time_us = now;
    }
    if (mask & sp::CONTROLLER_TEMP) state_.servo.controller_temp = static_cast<Real>(sample_.controller_temp);
    if (mask & sp::MOTOR_TEMP) state_.servo.motor_temp = static_cast<Real>(sample_.motor_temp);
    if (mask & sp::RUDDER) {
        state_.rudder.raw = static_cast<Real>(sample_.rudder);
        state_.rudder.raw_valid = !std::isnan(sample_.rudder);
        state_.rudder.raw_timestamp_us = now;
        state_.rudder.last_device = "servo";
    }
    if (mask & sp::FLAGS) {
        state_.servo.flags = sample_.flags;
        state_.servo.engaged = (sample_.flags & sp::ENGAGED) != 0;
        state_.servo.fault = (sample_.flags & (sp::OVERTEMP_FAULT | sp::OVERCURRENT_FAULT | sp::BADVOLTAGE_FAULT | sp::PORT_PIN_FAULT | sp::STARBOARD_PIN_FAULT)) != 0;
    }
}

void Servo::poll()
{
    state_.servo.brake_on = state_.servo.use_brake;

    if (fault()) {
        if ((state_.servo.flags & (sp::PORT_OVERCURRENT_FAULT | sp::STARBOARD_OVERCURRENT_FAULT)) == 0) ++state_.servo.faults;
        if ((state_.servo.flags & sp::OVERCURRENT_FAULT) != 0) {
            if (lastdir_ > 0) {
                set_flag(state_.servo.flags, sp::PORT_OVERCURRENT_FAULT, true);
                set_flag(state_.servo.flags, sp::STARBOARD_OVERCURRENT_FAULT, false);
            } else if (lastdir_ < 0) {
                set_flag(state_.servo.flags, sp::STARBOARD_OVERCURRENT_FAULT, true);
                set_flag(state_.servo.flags, sp::PORT_OVERCURRENT_FAULT, false);
            }
        }
        reset();
        stop();
        return;
    }

    double speed = state_.servo.command;
    if (!ap_enabled_ && speed == 0.0) disengaged_ = true;
    else if (speed != 0.0 || ap_enabled_) disengaged_ = false;

    if (disengaged_) {
        send_driver_params();
        queue_packet(sp::encode_disengage());
        state_.servo.state = "idle";
        return;
    }

    speed *= state_.servo.gain;
    if ((state_.servo.flags & (sp::PORT_OVERCURRENT_FAULT | sp::MAX_RUDDER_FAULT)) && speed > 0) { stop(); return; }
    if ((state_.servo.flags & (sp::STARBOARD_OVERCURRENT_FAULT | sp::MIN_RUDDER_FAULT)) && speed < 0) { stop(); return; }

    if (state_.servo.compensate_voltage && state_.servo.voltage > 0) speed *= 12.0 / state_.servo.voltage;

    const double min_speed = std::clamp(static_cast<double>(state_.servo.speed_min) / 100.0, 0.0, 1.0);
    const double max_speed = std::clamp(static_cast<double>(state_.servo.speed_max) / 100.0, 0.0, 1.0);
    if (speed > 0.0 && speed < min_speed) speed = min_speed;
    if (speed < 0.0 && speed > -min_speed) speed = -min_speed;
    speed = std::clamp(speed, -max_speed, max_speed);
    state_.servo.speed = static_cast<Real>(speed);

    if (speed == 0.0) {
        raw_command(0);
    } else {
        const double base = 0.2;
        const double span = 0.8;
        double out = base + std::abs(speed) * span;
        if (speed < 0) out = -out;
        raw_command(out);
    }

    int mul = (state_.servo.flags & (sp::PORT_OVERCURRENT_FAULT | sp::STARBOARD_OVERCURRENT_FAULT)) ? 2 : 1;
    send_driver_params(mul);

    if (ap_enabled_ != last_ap_enabled_) {
        last_ap_enabled_ = ap_enabled_;
        set_flag(state_.servo.flags, sp::PORT_OVERCURRENT_FAULT, false);
        set_flag(state_.servo.flags, sp::STARBOARD_OVERCURRENT_FAULT, false);
    }
}

} // namespace pypilot
