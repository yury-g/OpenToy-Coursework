idf_component_register(SRCS "App.cpp" "Camera.cpp" "../reference/Board.c" INCLUDE_DIRS "." "../reference" "imported" "core"
 REQUIRES esp_video human_face_detect esp_lcd esp_driver_i2c esp_driver_ledc esp_driver_uart esp_driver_gpio
 esp_lcd_ek79007 esp_lcd_touch_gt911 esp_lvgl_port lvgl esp_timer app_update esp_hw_support)
