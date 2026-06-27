#include "pypilot/pypilot.hpp"
#include <algorithm>
namespace pypilot { namespace sp=syslib::servo_protocol;
Servo::Servo(EventLoop& loop,PypilotState& state):loop_(loop),state_(state){raw_command(0);} void Servo::queue_packet(const std::array<uint8_t,4>& p){tx_.insert(tx_.end(),p.begin(),p.end());}
std::vector<uint8_t> Servo::take_driver_tx(){std::vector<uint8_t> out;out.swap(tx_);return out;} void Servo::send_driver_params(int){ }
void Servo::command(Real value){state_.servo.command=std::clamp(value,Real{-1},Real{1});} void Servo::raw_command(Real value){value=std::clamp(value,Real{-1},Real{1});state_.servo.raw_command=value;queue_packet(sp::encode_command(value));}
void Servo::stop(){raw_command(0);state_.servo.state="stop";} void Servo::reset(){queue_packet(sp::encode_reset());} void Servo::disengage(){queue_packet(sp::encode_disengage());state_.servo.engaged=false;} void Servo::set_autopilot_enabled(bool e){ap_enabled_=e;state_.servo.engaged=e;} bool Servo::fault() const{return state_.servo.fault;}
uint16_t Servo::feed_driver_bytes(const uint8_t* b,size_t n){uint16_t m=parser_.feed(b,n,sample_);if(m)apply_driver_sample(m);return m;} void Servo::apply_driver_sample(uint16_t m){if(m&sp::VOLTAGE)state_.servo.voltage=sample_.voltage; if(m&sp::CURRENT)state_.servo.current=sample_.current; if(m&sp::RUDDER){state_.rudder.raw=sample_.rudder;state_.rudder.raw_valid=true;state_.rudder.raw_timestamp_us=loop_.now_us();state_.rudder.last_device="servo";} if(m&sp::FLAGS){state_.servo.flags=sample_.flags;state_.servo.engaged=(sample_.flags&sp::ENGAGED)!=0;}}
void Servo::poll(){if(ap_enabled_)raw_command(state_.servo.command);else disengage();}
}
