#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>
#include "pypilot/syslib/data_model.hpp"
namespace pypilot::syslib::servo_protocol {
using Real = data_model::Real;
enum CommandCode : uint8_t { COMMAND_CODE=0xc7, RESET_CODE=0xe7, DISENGAGE_CODE=0x68, MAX_CURRENT_CODE=0x1e, MAX_CONTROLLER_TEMP_CODE=0xa4, MAX_MOTOR_TEMP_CODE=0x5a, RUDDER_MIN_CODE=0x2b, RUDDER_MAX_CODE=0x4d, MAX_SLEW_CODE=0x71, CLUTCH_PWM_AND_BRAKE_CODE=0x36 };
enum Telemetry : uint16_t { FLAGS=1, CURRENT=2, VOLTAGE=4, CONTROLLER_TEMP=32, MOTOR_TEMP=64, RUDDER=128, EEPROM=256 };
enum ServoFlags : uint32_t { OVERTEMP_FAULT=2, OVERCURRENT_FAULT=4, ENGAGED=8, PORT_PIN_FAULT=32, STARBOARD_PIN_FAULT=64, BADVOLTAGE_FAULT=128, MIN_RUDDER_FAULT=256, MAX_RUDDER_FAULT=512, PORT_OVERCURRENT_FAULT=65536, STARBOARD_OVERCURRENT_FAULT=131072 };
struct Packet { uint8_t code=0; uint16_t value=0; };
struct TelemetrySample { uint16_t mask=0; Real current=0; Real voltage=0; Real controller_temp=0; Real motor_temp=0; Real rudder=std::numeric_limits<Real>::quiet_NaN(); uint32_t flags=0; };
struct DriverParams { Real raw_max_current=Real{4.5f}; Real rudder_min=Real{-0.5f}; Real rudder_max=Real{0.5f}; Real max_controller_temp=60; Real max_motor_temp=60; Real max_slew_speed=28; Real max_slew_slow=34; Real clutch_pwm=100; bool brake=true; };
inline uint8_t crc8_update(uint8_t crc,uint8_t byte){crc^=byte;for(int i=0;i<8;++i)crc=(crc&0x80)?static_cast<uint8_t>((crc<<1)^0x31):static_cast<uint8_t>(crc<<1);return crc;}
inline uint8_t crc8(const uint8_t* data,size_t len){uint8_t crc=0xff;for(size_t i=0;i<len;++i)crc=crc8_update(crc,data[i]);return crc;}
inline std::array<uint8_t,4> encode(uint8_t code,uint16_t value){std::array<uint8_t,4> out{code,static_cast<uint8_t>(value&0xff),static_cast<uint8_t>((value>>8)&0xff),0};out[3]=crc8(out.data(),3);return out;}
inline bool decode(const uint8_t* bytes,Packet& packet){if(crc8(bytes,3)!=bytes[3])return false;packet.code=bytes[0];packet.value=static_cast<uint16_t>(bytes[1]|(bytes[2]<<8));return true;}
inline std::array<uint8_t,4> encode_command(Real command){command=std::clamp(command,Real{-1},Real{1});return encode(COMMAND_CODE,static_cast<uint16_t>((command+Real{1})*Real{1000}));}
inline std::array<uint8_t,4> encode_reset(){return encode(RESET_CODE,0);} inline std::array<uint8_t,4> encode_disengage(){return encode(DISENGAGE_CODE,0);} inline auto encode_reprogram(){return encode(RESET_CODE,0);} 
inline std::optional<std::array<uint8_t,4>> encode_param_packet(const DriverParams&,int){return std::nullopt;}
inline uint16_t apply_packet(const Packet& p,TelemetrySample& s){if(p.code==0xb3){s.voltage=static_cast<Real>(p.value)/Real{100};s.mask|=VOLTAGE;return VOLTAGE;} if(p.code==0x1c){s.current=static_cast<Real>(p.value)/Real{100};s.mask|=CURRENT;return CURRENT;} if(p.code==0xa7){s.rudder=static_cast<Real>(p.value)/Real{65472}-Real{0.5f};s.mask|=RUDDER;return RUDDER;} if(p.code==0x8f){s.flags=p.value;s.mask|=FLAGS;return FLAGS;} return 0;}
class Parser{public:uint16_t feed(const uint8_t* b,size_t n,TelemetrySample& s){uint16_t m=0;for(size_t i=0;i<n;++i)buf.push_back(b[i]);while(buf.size()>=4){Packet p;if(decode(buf.data(),p))m|=apply_packet(p,s);buf.erase(buf.begin(),buf.begin()+4);}return m;}private:std::vector<uint8_t> buf;};
}
