// SPDX-License-Identifier: MIT
#pragma once
namespace feather_water {
struct Down {float x,y;};
// Initial mounting contract: IMU XY plane parallel to the TFT, Y along its
// long edge. Final signs must be checked on the physical mounted assembly.
inline Down screenDown(float ax,float ay){return {-ax,-ay};}
}
