/**
 * @file watchface_app.c
 * @brief Simple watchface displaying current time (hh:mm)
 */

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#include "../../lib/rtc.h"
#include "../app_interface.h"
#include "lvgl.h"

LOG_MODULE_REGISTER(watchface_app, LOG_LEVEL_INF);

static lv_obj_t *hour_label = NULL;
static lv_obj_t *min_label = NULL;
static lv_obj_t *colon_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_timer_t *update_timer = NULL;
static const struct device *rtc = NULL;

static void update_time_cb(lv_timer_t *timer) {
  LV_UNUSED(timer);

  if (hour_label == NULL || rtc == NULL) {
    return;
  }

  struct rtc_time tm;
  int ret = rtc_get_time(rtc, &tm);
  LOG_DBG("RTC time: %02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);

  if (ret < 0) {
    LOG_ERR("Failed to get RTC time: %d", ret);
    return;
  }

  // Update hour and minute labels
  if (hour_label && min_label) {
    lv_label_set_text_fmt(hour_label, "%02d", tm.tm_hour);
    lv_label_set_text_fmt(min_label, "%02d", tm.tm_min);
  }

  // Update date label
  if (date_label) {
    const char *months[] = {"January",   "February", "March",    "April",
                            "May",       "June",     "July",     "August",
                            "September", "October",  "November", "December"};
    const char *month_name = months[tm.tm_mon];

    // Format: "Month Dth Year" (e.g., "March 3rd 2021")
    const char *suffix = "th";
    int day = tm.tm_mday;
    if (day % 10 == 1 && day != 11) {
      suffix = "st";
    } else if (day % 10 == 2 && day != 12) {
      suffix = "nd";
    } else if (day % 10 == 3 && day != 13) {
      suffix = "rd";
    }

    lv_label_set_text_fmt(date_label, "%s %d%s %d", month_name, day, suffix,
                          tm.tm_year + 1900);
  }
}

static void watchface_app_init(void) {
  LOG_INF("Watchface app init");

  // Get RTC device
  rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
  if (!device_is_ready(rtc)) {
    LOG_ERR("RTC device not ready");
    rtc = NULL;
  }

  // Clean screen and set a white background
  lv_obj_clean(lv_scr_act());

  // Style: larger font for readability
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_text_font(&style, &lv_font_montserrat_48);

  // Create hour label with black background and white text
  hour_label = lv_label_create(lv_scr_act());
  lv_label_set_text(hour_label, "--");
  lv_obj_align(hour_label, LV_ALIGN_CENTER, -50, -20);
  lv_obj_add_style(hour_label, &style, 0);
  
  static lv_style_t hour_style;
  lv_style_init(&hour_style);
  lv_style_set_bg_color(&hour_style, lv_color_black());
  lv_style_set_bg_opa(&hour_style, LV_OPA_COVER);
  lv_style_set_text_color(&hour_style, lv_color_white());
  lv_style_set_pad_all(&hour_style, 8);
  lv_obj_add_style(hour_label, &hour_style, 0);

  // Create colon separator (will toggle)
  colon_label = lv_label_create(lv_scr_act());
  lv_label_set_text(colon_label, ":");
  lv_obj_align(colon_label, LV_ALIGN_CENTER, 0, -20);
  lv_obj_add_style(colon_label, &style, 0);

  // Create minute label
  min_label = lv_label_create(lv_scr_act());
  lv_label_set_text(min_label, "--");
  lv_obj_align(min_label, LV_ALIGN_CENTER, 50, -20);
  lv_obj_add_style(min_label, &style, 0);

  // Create date label
  date_label = lv_label_create(lv_scr_act());
  lv_label_set_text(date_label, "---- -- ----");
  lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 50);

  static lv_style_t date_style;
  lv_style_init(&date_style);
  lv_style_set_text_font(&date_style, &lv_font_montserrat_16);
  lv_obj_add_style(date_label, &date_style, 0);

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
  date_label = NULL;
  rtc = NULL;
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
