#pragma once

#include <string>
#include "pypilot/pypilot.hpp"

namespace pypilot {

class Pilot {
public:
    Pilot(PypilotState& state, std::string name);
    virtual ~Pilot() = default;
    const std::string& name() const { return name_; }
    virtual void compute_heading();
    virtual void process();
protected:
    PypilotState& state_;
    std::string name_;
};

class BasicPilot : public Pilot { public: explicit BasicPilot(PypilotState& state); void process() override; };
class SimplePilot : public Pilot { public: explicit SimplePilot(PypilotState& state); void process() override; };
class GpsPilot : public Pilot { public: explicit GpsPilot(PypilotState& state); void compute_heading() override; };
class WindPilot : public Pilot { public: explicit WindPilot(PypilotState& state); void compute_heading() override; };
class AbsolutePilot : public Pilot { public: explicit AbsolutePilot(PypilotState& state); };
class RatePilot : public Pilot { public: explicit RatePilot(PypilotState& state); };
class DeadzonePilot : public Pilot { public: explicit DeadzonePilot(PypilotState& state); };
class VmgPilot : public Pilot { public: explicit VmgPilot(PypilotState& state); };
class FuzzyPilot : public Pilot { public: explicit FuzzyPilot(PypilotState& state); };
class IntellectPilot : public Pilot { public: explicit IntellectPilot(PypilotState& state); };
class LearningPilot : public Pilot { public: explicit LearningPilot(PypilotState& state); };
class AutotunePilot : public Pilot { public: explicit AutotunePilot(PypilotState& state); };

} // namespace pypilot
