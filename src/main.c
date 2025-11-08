/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <lvgl.h>
#include <stdio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app/app_interface.h"
#include "app/app_manager.h"
#include "buttons.h"

extern int sensor_init(void);
extern void epd_display_init(void);
extern IApp CounterApp;
extern IApp ImagesApp;

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/**
 * @brief Initialize LVGL display
 */
static int lvgl_display_init(void) {
  const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
  if (!device_is_ready(display)) {
    LOG_ERR("Display device not ready");
    return -1;
  }

  LOG_INF("Display is ready");
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);

  return 0;
}

int main(void) {
  LOG_INF("Starting Watchy Zephyr App with App Framework");

  // Initialize button subsystem
  button_init();

  // Initialize LVGL display
  if (lvgl_display_init() < 0) {
    LOG_ERR("Failed to initialize display");
    return -1;
  }

  // Register applications
  app_manager_register(&ImagesApp);
  app_manager_register(&CounterApp);

  // Launch the first app (CounterApp)
  LOG_INF("Launching counter app");
  app_manager_launch(0);

  // Main event loop
  while (1) {
    uint32_t sleep_time = lv_timer_handler();
    if (sleep_time >= 1000) {
      sleep_time = 1000;
    }
    k_sleep(K_MSEC(sleep_time));
  }

  return 0;
}
