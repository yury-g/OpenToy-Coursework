// SPDX-License-Identifier: MIT
#pragma once
#if __has_include("BuildRevision.h")
#include "BuildRevision.h"
#endif
#ifndef OPENTOY_BUILD_REV
#define OPENTOY_BUILD_REV "local"
#endif
namespace rlcd_eyes {
constexpr char appId[]="animatronic-eyes-1",appName[]="EYES1";
constexpr char version[]="0.2.1",created[]="2026-11-17";
constexpr char target[]="waveshare-esp32-s3-rlcd-4.2";
constexpr char batterySource[]="GPIO4 ADC1_CH3 calibrated x3 voltage";
constexpr char revision[]=OPENTOY_BUILD_REV;
}
