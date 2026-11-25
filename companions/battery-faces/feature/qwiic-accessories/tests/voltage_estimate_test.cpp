#include "../firmware/FeatherFace/VoltageEstimate.h"
#include <cassert>
#include <cmath>
int main(){assert(voltagePercent(4.2f)==100);assert(voltagePercent(3.3f)==0);assert(voltagePercent(3.8f)==50);assert(voltagePercent(3.95f)>70&&voltagePercent(3.95f)<80);assert(std::isnan(voltagePercent(0)));assert(std::isnan(voltagePercent(5)));assert(std::isnan(voltagePercent(NAN)));}
