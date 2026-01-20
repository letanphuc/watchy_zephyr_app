/**
 * @file segments_wf_app.c
 * @brief Watchface with seven segments font displaying time (hh:mm)
 */

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#include "../../lib/rtc.h"
#include "../app_interface.h"
#include "lvgl.h"

LOG_MODULE_REGISTER(segments_wf_app, LOG_LEVEL_INF);

// Declare the generated seven segments font
LV_FONT_DECLARE(seven_segments_64);

static lv_obj_t *hour_label = NULL;
static lv_obj_t *min_label = NULL;
static lv_obj_t *colon_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *weekday_rects[7] = {NULL};
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
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *month_name = months[tm.tm_mon];
    const char *day_name = days[tm.tm_wday];

    // Format: "Month Dth Year\nDayOfWeek" (e.g., "Mar 3rd 2021\nMon")
    const char *suffix = "th";
    int day = tm.tm_mday;
    if (day % 10 == 1 && day != 11) {
      suffix = "st";
    } else if (day % 10 == 2 && day != 12) {
      suffix = "nd";
    } else if (day % 10 == 3 && day != 13) {
      suffix = "rd";
    }

    lv_label_set_text_fmt(date_label, "%s - %s %d%s %d", day_name, month_name,
                          day, suffix, tm.tm_year + 1900);
  }

  // Update week day indicators
  for (int i = 0; i < 7; i++) {
    if (weekday_rects[i]) {
      if (i == tm.tm_wday) {
        lv_obj_set_style_bg_color(weekday_rects[i], lv_color_black(), 0);
      } else {
        lv_obj_set_style_bg_color(weekday_rects[i], lv_color_white(), 0);
      }
    }
  }
}

static void segments_wf_app_init(void) {
  LOG_INF("Segments watchface app init");

  // Get RTC device
  rtc = DEVICE_DT_GET(DT_ALIAS(rtc));
  if (!device_is_ready(rtc)) {
    LOG_ERR("RTC device not ready");
    rtc = NULL;
  }

  // Clean screen and set a white background
  lv_obj_clean(lv_scr_act());

  // Style: seven segments font for time display
  static lv_style_t time_style;
  lv_style_init(&time_style);
  lv_style_set_text_font(&time_style, &seven_segments_64);

  // Create hour label with black background and white text
  hour_label = lv_label_create(lv_scr_act());
  lv_label_set_text(hour_label, "00");
  lv_obj_align(hour_label, LV_ALIGN_CENTER, -50, -20);
  lv_obj_add_style(hour_label, &time_style, 0);

  static lv_style_t hour_style;
  lv_style_init(&hour_style);
  lv_style_set_bg_color(&hour_style, lv_color_black());
  lv_style_set_bg_opa(&hour_style, LV_OPA_COVER);
  lv_style_set_text_color(&hour_style, lv_color_white());
  lv_style_set_pad_all(&hour_style, 8);
  lv_style_set_radius(&hour_style, 10);
  lv_obj_add_style(hour_label, &hour_style, 0);

  // Create colon separator
  colon_label = lv_label_create(lv_scr_act());
  lv_label_set_text(colon_label, ":");
  lv_obj_align(colon_label, LV_ALIGN_CENTER, 0, -20);
  lv_obj_add_style(colon_label, &time_style, 0);

  // Create minute label
  min_label = lv_label_create(lv_scr_act());
  lv_label_set_text(min_label, "00");
  lv_obj_align(min_label, LV_ALIGN_CENTER, 50, -20);
  lv_obj_add_style(min_label, &time_style, 0);

  // Create date label with regular font
  date_label = lv_label_create(lv_scr_act());
  lv_label_set_text(date_label, "---- -- ----");
  lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 50);

  static lv_style_t date_style;
  lv_style_init(&date_style);
  lv_style_set_text_font(&date_style, &lv_font_montserrat_16);
  lv_obj_add_style(date_label, &date_style, 0);

  // Create 7 rectangles for week day indicators
  static lv_style_t rect_style;
  lv_style_init(&rect_style);
  lv_style_set_radius(&rect_style, 3);
  lv_style_set_border_width(&rect_style, 1);
  lv_style_set_border_color(&rect_style, lv_color_black());

  int rect_width = 12;
  int rect_height = 12;
  int spacing = 4;
  int total_width = (rect_width * 7) + (spacing * 6);
  int start_x = -total_width / 2;

  for (int i = 0; i < 7; i++) {
    weekday_rects[i] = lv_obj_create(lv_scr_act());
    lv_obj_set_size(weekday_rects[i], rect_width, rect_height);
    lv_obj_align(weekday_rects[i], LV_ALIGN_CENTER,
                 start_x + (i * (rect_width + spacing)) + (rect_width / 2), 80);
    lv_obj_add_style(weekday_rects[i], &rect_style, 0);
    lv_obj_set_style_bg_color(weekday_rects[i], lv_color_white(), 0);
  }

  // Update immediately, then every second
  update_time_cb(NULL);
  update_timer = lv_timer_create(update_time_cb, 1000, NULL);
}

static void segments_wf_app_deinit(void) {
  LOG_INF("Segments watchface app deinit");
  if (update_timer) {
    lv_timer_del(update_timer);
    update_timer = NULL;
  }
  lv_obj_clean(lv_scr_act());
  hour_label = NULL;
  min_label = NULL;
  colon_label = NULL;
  date_label = NULL;
  for (int i = 0; i < 7; i++) {
    weekday_rects[i] = NULL;
  }
  rtc = NULL;
}

static void segments_wf_app_handle_event(input_event_t *ev) {
  LV_UNUSED(ev);
  // No input handling needed for simple watchface
}

IApp SegmentsWatchfaceApp = {
    .init = segments_wf_app_init,
    .deinit = segments_wf_app_deinit,
    .handle_event = segments_wf_app_handle_event,
};
