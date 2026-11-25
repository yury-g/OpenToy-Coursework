// SPDX-License-Identifier: MIT
#pragma once
#if __has_include("BuildRevision.h")
#include "BuildRevision.h"
#endif
namespace waveshare_eyes {
constexpr char appId[]="animatronic-eyes-1", appName[]="EYES1";
constexpr char version[]="1.0.2", created[]="2026-11-17";
constexpr char target[]="waveshare-esp32-s3-1.85c-box-v2", batterySource[]="GPIO8 ADC x3 voltage";
#ifndef OPENTOY_BUILD_REV
#define OPENTOY_BUILD_REV "local"
#endif
constexpr char revision[]=OPENTOY_BUILD_REV;
}
