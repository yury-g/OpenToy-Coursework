# Vendor driver provenance

ST77916 driver (`esp_lcd_st77916.c/.h`) and case2 initialization table (`PanelInit.h`) come from [Waveshare's 1.85C repository](https://github.com/waveshareteam/ESP32-S3-Touch-LCD-1.85C/tree/8ead4a96bf3a278fc4ebd8ef4768657e17fa2880/Arduino/examples/01_lvgl_example), commit `8ead4a96bf3a278fc4ebd8ef4768657e17fa2880`.

ES8311 files come from the same commit's `Arduino/examples/03_audio_out_no_tf`. Original Espressif copyright notices remain. `LICENSE.vendor` contains the upstream Apache-2.0 license.

Local adaptations: checked Wire read count, suppressed raw I2C printf errors, checked codec allocation failure, and kept ADC/PGA powered down through register 0x0e=0x6a (its documented power-down default), checked by the defined ADC/PGA bits rather than reserved bits. No microphone configuration call is made. The separate ES7210 microphone encoder is never initialized.

`Board.cpp` uses a new bounded adapter rather than the vendor demonstration loop: releases the slow register-read IO object, accepts only verified panel case2, waits for each strip DMA completion, latches display faults, checks touch transaction lengths, and caps speaker output. It retains original vendor driver/table attribution; no demo audio, networking, cloud, LVGL or microphone code is included.

Codec register meanings were checked against the manufacturer’s [ES8311 datasheet, revision 10.0, page 18](https://files.waveshare.com/wiki/common/ES8311.DS.pdf). Reserved bits do not necessarily echo writes.
