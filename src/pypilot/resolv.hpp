#pragma once

#include <cmath>
#include "pypilot/syslib/data_model.hpp"

namespace pypilot {
using Real = syslib::data_model::Real;
inline Real resolv(Real angle) { while (angle < Real{-180}) angle += Real{360}; while (angle >= Real{180}) angle -= Real{360}; return angle; }
inline Real heading_error(Real command_deg, Real heading_deg) { return resolv(command_deg - heading_deg); }
inline Real compass_normalize(Real angle) { angle = std::fmod(angle, Real{360}); if (angle < Real{0}) angle += Real{360}; return angle; }
} // namespace pypilot
