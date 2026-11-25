// SPDX-License-Identifier: MIT
#pragma once
#include <cstdint>
namespace waveshare_eyes {
constexpr int stripRows=60;
bool boardBegin();
bool readTouch(uint8_t* bytes);
int readBattery();
bool displayStrip(int y,const uint16_t* pixels); // exactly 360 x stripRows, waits for DMA completion
uint16_t* displayBuffer();
bool audioBegin();
bool playSound(unsigned eye,unsigned volume=2);
bool setBrightness(unsigned level);
void stopSound();
void pumpSound();
bool ampEnabled();
bool audioAvailable();
extern int audioStage,audioResult,codecPower,batteryRaw;
extern uint32_t displayTransfers,displayErrors,audioPlays,audioErrors;
}
