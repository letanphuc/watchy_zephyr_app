# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

# Include LVGL image conversion function
include(cmake/lvgl_image.cmake)

# Include LVGL font conversion function
include(cmake/lvgl_font.cmake)

include_directories(src/EPD_W21/)
include_directories(src/)

# Convert all images in src/app/images/res
file(GLOB IMAGE_FILES "src/app/images/res/*.png")
if(IMAGE_FILES)
    lvgl_add_images(
        IMAGES ${IMAGE_FILES}
        COLOR_FORMAT RGB565
        RGB565_DITHER
    )
endif()

# Convert seven segments font with number characters and colon
lvgl_add_font(
    FONT_FILE fonts/seven_segments.ttf
    SIZE 64
    BPP 1
    RANGE "0x30-0x3A"
)

# Convert Vietnamese font for notifications if the source is available
set(VIETNAMESE_FONT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/fonts/NotoSans_Condensed-Medium.ttf")
if(EXISTS "${VIETNAMESE_FONT_FILE}")
    lvgl_add_font(
        FONT_FILE ${VIETNAMESE_FONT_FILE}
        SIZE 20
        BPP 4
        FORMAT lvgl
        RANGE "0x0020-0x007F,0x00A0-0x00FF,0x0102,0x0103,0x0110,0x0111,0x0128,0x0129,0x0168,0x0169,0x01A0,0x01A1,0x01AF,0x01B0,0x1EA0-0x1EF9"
        OUTPUT_NAME font_vi_20
        NO_COMPRESS
        NO_PREFILTER
    )
    target_compile_definitions(app PRIVATE HAS_FONT_VI_20)
else()
    message(WARNING "NotoSans_Condensed-Medium.ttf not found; skipping Vietnamese font generation")
endif()

add_compile_definitions(LV_LVGL_H_INCLUDE_SIMPLE)

target_sources(app PRIVATE 
    src/main.c 
    src/network.c 
    src/buttons.c 
    src/battery.c 
    src/sensors.c
    ${LVGL_FONT_SOURCES}
    src/lib/rtc.c
    src/lib/ancs.c
    src/app/app_manager.c
    src/app/watchface/watchface_app.c
    src/app/watchface/segments_wf_app.c
    src/app/counter/counter_app.c
    src/app/notification/notification_app.c
    src/app/images/images_app.c
    src/app/gpio_event.c
    ${LVGL_IMAGE_SOURCES}
)
