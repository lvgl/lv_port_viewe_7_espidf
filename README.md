# LVGL ported to VIEWE 7.0"

## Overview

Short overview

## Buy

You can purchase Viewe ESP32 7 Inch 800×480 from https://viewedisplay.com/product/esp32-7-inch-800x480-rgb-ips-tft-display-touch-screen-arduino-lvgl-uart/

## Benchmark

Describe the default buffering and other configuration.
esp_lcd_rgb_panel two framebuffers are used as LVGL draw buffers with LV_DISPLAY_RENDER_MODE_DIRECT mode

YouTube video link

## Specification

### CPU and Memory
- **MCU:** ESP32-S3 240Mhz
- **RAM:** 512 KB internal, 8MB external PSRAM
- **Flash:** 16MB External Flash
- **GPU:** None

### Display and Touch
- **Resolution:** 800x480
- **Display Size:** 7"
- **Interface:** RGB (EK9716BD3+EK73002AB2)
- **Color Depth:** 24-bit
- **Technology:** TN
- **DPI:** 133px/inch
- **Touch Pad:** Capacitive (GT911)

### Connectivity
- WS2812B single LED
- Micro SD card slot

## Getting started

### Hardware setup
- jumpers, switches
- connect the display
- Connect Type-C cable to UART (recommended) or USB port

### Software setup
- Install drivers if needed
- Install the VS Code IDE & PlatformIO extension

### Run the project
- Clone this repository: 
- Open the code folder using VS Code. PlatformIO needs to be installed. ESP-IDF will automatically be installed if not present
- Configure the project. Click on the gear icon (SDK Configuration editor)
- Build the project. Click on the wrench icon (Build Project)
- Run or Debug. Alternatively click on the fire icon (ESP-IDF: Build, Flash & Monitor) to run flash and debug the code

### Debugging
- Debug, ESP Logging Library `ESP_LOGE, ESP_LOGI ...`?
- Other?
-
## Notes

Other notes, e.g. different configs, optimization opportunities, adding other libraries to the project, etc

## Contribution and Support

If you find any issues with the development board feel free to open an Issue in this repository. For LVGL related issues (features, bugs, etc) please use the main [lvgl repository](https://github.com/lvgl/lvgl).

If you found a bug and found a solution too please send a Pull request. If you are new to Pull requests refer to [Our Guide](https://docs.lvgl.io/master/CONTRIBUTING.html#pull-request) to learn the basics.
