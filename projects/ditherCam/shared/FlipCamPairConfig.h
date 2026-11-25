// SPDX-License-Identifier: MIT
#pragma once

#if defined(__has_include)
#if __has_include("private-localPairConfig.h")
#include "private-localPairConfig.h"
#elif __has_include("privatePairConfig.h")
#include "privatePairConfig.h"
#else
#include "CompileConfig.example.h"
#endif
#else
#include "CompileConfig.example.h"
#endif

#ifndef FLIPCAM_PAIR_CONFIG_IS_DUMMY
#define FLIPCAM_PAIR_CONFIG_IS_DUMMY 0
#endif

#ifndef FLIPCAM_WIFI_SSID
#error "FLIPCAM_WIFI_SSID must be defined"
#endif
#ifndef FLIPCAM_WIFI_PSK
#error "FLIPCAM_WIFI_PSK must be defined"
#endif
#ifndef FLIPCAM_PAIR_TOKEN
#error "FLIPCAM_PAIR_TOKEN must be defined"
#endif

#ifndef FLIPCAM_STREAM_PORT
#define FLIPCAM_STREAM_PORT 8081
#endif
