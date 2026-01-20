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
#include "lib/ancs.h"

extern int sensor_init(void);
extern void epd_display_init(void);
extern int init_net(void);
extern IApp CounterApp;
extern IApp ImagesApp;
extern IApp WatchfaceApp;
extern IApp SegmentsWatchfaceApp;
extern IApp NotificationApp;

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

  display_blanking_off(display);

  LOG_INF("Display is ready");
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);

  return 0;
}

void on_new_notification(const struct ancs_notification *notif) {
  LOG_INF("New Notification:");
  LOG_INF("  UID: 0x%x", notif->source.notification_uid);
  LOG_INF("  App ID: %s", notif->app_identifier);
  LOG_INF("  Title: %s", notif->title);
  LOG_INF("  Subtitle: %s", notif->subtitle);
  LOG_INF("  Message: %s", notif->message);
  LOG_INF("  Date: %s", notif->date);
  LOG_INF("  Positive Action: %s", notif->positive_action_label);
}
void on_notification_removed(uint32_t uid) {
  LOG_INF("Notification Removed: UID=0x%x", uid);
}
struct ancs_callbacks ancs_cbs = {
    .on_new_notification = on_new_notification,
    .on_notification_removed = on_notification_removed,
};

int main(void) {
  LOG_INF("Starting Watchy Zephyr App with App Framework");

  // Initialize ANCS client
  ancs_client_init();
  ancs_register_cb(&ancs_cbs);

  // Initialize button subsystem
  button_init();
  // init_net();

  // Initialize LVGL display
  if (lvgl_display_init() < 0) {
    LOG_ERR("Failed to initialize display");
    return -1;
  }

  // Register applications (launch segments watchface by default)
  // app_manager_register(&SegmentsWatchfaceApp);
  // app_manager_register(&WatchfaceApp);
  app_manager_register(&NotificationApp);
  // app_manager_register(&ImagesApp);
  // app_manager_register(&CounterApp);

  LOG_INF("Launching segments watchface app");
  app_manager_launch(0);

  // Main event loop
  while (1) {
    LOG_DBG("Main loop tick");
    uint32_t sleep_time = lv_timer_handler();
    if (sleep_time >= 1000) {
      sleep_time = 1000;
    }
    k_sleep(K_MSEC(sleep_time));
  }

  return 0;
}
