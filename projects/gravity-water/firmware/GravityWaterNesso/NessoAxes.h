// SPDX-License-Identifier: MIT
#pragma once
#include "src/fluid/Gravity.h"
namespace nesso_water {
// Nesso portrait rotation 0: measured upright on bottom edge gives accel +X.
// Owner confirmed bar orientation; the sensor is rotated relative to the glass.
// This hardware mapping belongs here, outside the reusable particle physics.
inline gravity_water::Point screenDown(float ax,float ay) {return {ay,ax};}
}
