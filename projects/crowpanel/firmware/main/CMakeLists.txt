idf_component_register(SRCS "App.cpp" "Board.c" INCLUDE_DIRS "." "imported"
    REQUIRES esp_lcd esp_driver_i2c esp_driver_ledc esp_driver_uart esp_driver_gpio
    esp_lcd_ek79007 esp_lcd_touch_gt911 esp_lvgl_port lvgl esp_timer app_update esp_hw_support)
