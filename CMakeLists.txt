# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

# Include LVGL image conversion function
include(cmake/lvgl_image.cmake)

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

add_compile_definitions(LV_LVGL_H_INCLUDE_SYSTEM)

target_sources(app PRIVATE 
    src/main.c 
    src/network.c 
    src/buttons.c 
    src/battery.c 
    src/sensors.c
    src/lib/rtc.c
    src/app/app_manager.c
    src/app/watchface/watchface_app.c
    src/app/counter/counter_app.c
    src/app/images/images_app.c
    src/app/gpio_event.c
    ${LVGL_IMAGE_SOURCES}
)
