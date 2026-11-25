// SPDX-License-Identifier: MIT
// Public example pair configuration with non-working dummy credentials.
// Coordinator creates private-localPairConfig.h (or privatePairConfig.h) in
// a local private build copy before compiling production firmware.
// NEVER commit real PSKs or tokens to this repository.
#pragma once

#define FLIPCAM_PAIR_CONFIG_IS_DUMMY 1

#define FLIPCAM_WIFI_SSID      "FlipCam-Example-AP"
#define FLIPCAM_WIFI_PSK       "example-change-me-psk"
#define FLIPCAM_WIFI_CHANNEL   6

#define FLIPCAM_RLCD_IP_A      192
#define FLIPCAM_RLCD_IP_B      168
#define FLIPCAM_RLCD_IP_C      4
#define FLIPCAM_RLCD_IP_D      1

#define FLIPCAM_XIAO_IP_A      192
#define FLIPCAM_XIAO_IP_B      168
#define FLIPCAM_XIAO_IP_C      4
#define FLIPCAM_XIAO_IP_D      2

#define FLIPCAM_PEER_PORT      8080
#define FLIPCAM_WEB_PORT       80
#define FLIPCAM_PAIR_TOKEN     "example-dummy-pair-token"
