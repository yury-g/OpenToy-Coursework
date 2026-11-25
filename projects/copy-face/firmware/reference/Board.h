// SPDX-License-Identifier: MIT
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t board_display_begin(void);
esp_err_t board_brightness(unsigned percent);
esp_err_t board_touch_begin(void);
esp_err_t board_touch_read(uint16_t* x,uint16_t* y,uint8_t* count);
#ifdef __cplusplus
}
#endif
