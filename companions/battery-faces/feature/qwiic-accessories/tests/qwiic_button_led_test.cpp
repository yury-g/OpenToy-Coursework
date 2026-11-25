#include <cassert>
#include <cstdio>
#include "../firmware/FeatherFace/QwiicButtonLed.h"

int main() {
    struct Example {
        bool lit, pressed;
        uint8_t command;
        bool active, breathing;
        uint8_t expected;
    };
    const Example examples[] = {
        {true, false, 0, true, true, 0},       // Motor rest is completely dark.
        {true, false, 1, true, true, 6},
        {true, false, 12, true, true, 72},
        {true, false, 20, true, true, 120},
        {true, false, 28, true, true, 168},
        {true, false, 32, true, true, 192},
        {true, false, 64, true, true, 192},
        {true, false, 255, true, true, 192},
        {true, false, 0, true, false, 16},
        {true, false, 20, true, false, 76},
        {true, false, 64, true, false, 208},
        {true, false, 79, true, false, 253},
        {true, false, 80, true, false, 255},
        {true, false, 255, true, false, 255},  // No uint8_t overflow.
        {true, false, 255, false, false, 16}, // Quiet locator ignores stale drive.
        {true, false, 255, false, true, 16},
        {true, true, 0, true, true, 128},     // Physical press overrides dark rest.
        {true, true, 255, true, false, 128},
        {true, true, 0, false, false, 128},
        {false, true, 255, true, true, 0},    // Explicit LED disable overrides press.
        {false, false, 255, true, false, 0},
        {false, false, 0, false, false, 0},
    };
    for (const auto &example : examples) {
        assert(qwiicButtonBrightness(example.lit, example.pressed, example.command,
                                     example.active, example.breathing) == example.expected);
    }
    puts("Qwiic LED: breathing modulation, dark rest, limits, press override and idle locator PASS");
}
