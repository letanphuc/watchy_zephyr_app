/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>
#include <lvgl_input_device.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, 4);

#include "cat_image.h"

extern const lv_image_dsc_t cat_200x200;

int display_main(void) {
  LOG_INF("Display main started");
  const struct device* display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display)) {
    return 0;
  }
  LOG_INF("Display is ready");

  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);
  lv_obj_t* label = lv_label_create(lv_screen_active());
  LOG_INF("Label created at %p", label);
  lv_label_set_text(label, "Hello world!");
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_t* icon = lv_image_create(lv_screen_active());
  lv_image_set_src(icon, &cat_200x200);
  lv_obj_align_to(icon, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  LOG_INF("Display blanking off");

  while (1) {
    uint32_t t = lv_timer_handler();
    k_sleep(K_MSEC(t));
  }
}
