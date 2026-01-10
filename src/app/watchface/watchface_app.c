/**
 * @file watchface_app.c
 * @brief Simple watchface displaying current time (hh:mm)
 */

#include <time.h>
#include <zephyr/logging/log.h>

#include "../app_interface.h"
#include "lvgl.h"

LOG_MODULE_REGISTER(watchface_app, LOG_LEVEL_INF);

static lv_obj_t *hour_label = NULL;
static lv_obj_t *min_label = NULL;
static lv_obj_t *colon_label = NULL;
static lv_obj_t *progress_bar = NULL;
static lv_timer_t *update_timer = NULL;

static void update_time_cb(lv_timer_t *timer) {
  LV_UNUSED(timer);

  if (progress_bar == NULL) {
    return;
  }

  time_t now = time(NULL);
  struct tm *ptm = localtime(&now);
  if (ptm == NULL) {
    return;
  }
  struct tm tm_now = *ptm;

  // Update hour and minute labels
  if (hour_label && min_label) {
    lv_label_set_text_fmt(hour_label, "%02d", tm_now.tm_hour);
    lv_label_set_text_fmt(min_label, "%02d", tm_now.tm_min);
  }
  
  // Toggle colon visibility based on seconds (blink effect)
  if (colon_label) {
    lv_obj_set_style_opa(colon_label, (tm_now.tm_sec % 2 == 0) ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
  }
  
  // Update progress bar (0-59 seconds)
  lv_bar_set_value(progress_bar, tm_now.tm_sec, LV_ANIM_OFF);
}

static void watchface_app_init(void) {
  LOG_INF("Watchface app init");

  // Clean screen and set a white background
  lv_obj_clean(lv_scr_act());

  // Style: larger font for readability
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_text_font(&style, &lv_font_montserrat_24);

  // Create hour label
  hour_label = lv_label_create(lv_scr_act());
  lv_label_set_text(hour_label, "--");
  lv_obj_align(hour_label, LV_ALIGN_CENTER, -20, -10);
  lv_obj_add_style(hour_label, &style, 0);

  // Create colon separator (will toggle)
  colon_label = lv_label_create(lv_scr_act());
  lv_label_set_text(colon_label, ":");
  lv_obj_align(colon_label, LV_ALIGN_CENTER, 0, -10);
  lv_obj_add_style(colon_label, &style, 0);

  // Create minute label
  min_label = lv_label_create(lv_scr_act());
  lv_label_set_text(min_label, "--");
  lv_obj_align(min_label, LV_ALIGN_CENTER, 20, -10);
  lv_obj_add_style(min_label, &style, 0);

  // Create progress bar for seconds (0-59)
  progress_bar = lv_bar_create(lv_scr_act());
  lv_obj_set_size(progress_bar, 150, 10);
  lv_obj_align(progress_bar, LV_ALIGN_CENTER, 0, 20);
  lv_bar_set_range(progress_bar, 0, 59);
  lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);

  // Update immediately, then every second
  update_time_cb(NULL);
  update_timer = lv_timer_create(update_time_cb, 1000, NULL);
}

static void watchface_app_deinit(void) {
  LOG_INF("Watchface app deinit");
  if (update_timer) {
    lv_timer_del(update_timer);
    update_timer = NULL;
  }
  lv_obj_clean(lv_scr_act());
  hour_label = NULL;
  min_label = NULL;
  colon_label = NULL;
  progress_bar = NULL;
}

static void watchface_app_handle_event(input_event_t *ev) {
  LV_UNUSED(ev);
  // No input handling needed for simple watchface
}

IApp WatchfaceApp = {
    .init = watchface_app_init,
    .deinit = watchface_app_deinit,
    .handle_event = watchface_app_handle_event,
};
