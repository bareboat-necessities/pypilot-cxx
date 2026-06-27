#include "pypilot/pypilot.hpp"
#include <algorithm>
#include <cmath>
namespace pypilot {
Rudder::Rudder(PypilotState& state):state_(state){update_minmax();}
void Rudder::update_minmax(){auto& r=state_.rudder; Real s=std::fabs(r.scale)<Real{0.01f}?Real{100}:r.scale; r.min_raw=std::clamp((-r.range-r.offset)/s,Real{-0.5f},Real{0.5f}); r.max_raw=std::clamp((r.range-r.offset)/s,Real{-0.5f},Real{0.5f}); if(r.min_raw>r.max_raw)std::swap(r.min_raw,r.max_raw);}
bool Rudder::invalid() const{return !state_.rudder.angle.valid;}
bool Rudder::update(Real raw,std::string_view device,uint64_t now){auto& r=state_.rudder; if(device!=r.last_device){r.last_device=std::string(device); reset(); return false;} if(std::isnan(raw)){reset();return false;} r.raw=raw; r.raw_valid=true; r.raw_timestamp_us=now; update_minmax(); Real angle=r.scale*raw+r.offset+r.nonlinearity*(r.min_raw-raw)*(r.max_raw-raw); r.angle.value=static_cast<Real>(std::round(angle*Real{100})/Real{100}); r.angle.timestamp_us=now; r.angle.source=SensorSource::servo; r.angle.valid=true; return true;}
void Rudder::reset(){state_.rudder.angle.valid=false; state_.rudder.speed.valid=false; state_.rudder.raw_valid=false;}
void Rudder::poll(uint64_t now){auto& r=state_.rudder; update_minmax(); if(r.raw_valid)update(r.raw,r.last_device.empty()?std::string_view("servo"):std::string_view(r.last_device),now);}
}
