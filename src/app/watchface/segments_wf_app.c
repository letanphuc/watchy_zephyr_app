/**
 * @file segments_wf_app.c
 * @brief Watchface with seven segments font displaying time (hh:mm)
 */

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

#include "../../lib/ancs.h"
#include "../../lib/rtc.h"
#include "../app_interface.h"
#include "lvgl.h"

LOG_MODULE_REGISTER(segments_wf_app, LOG_LEVEL_INF);

// Declare the generated seven segments font
LV_FONT_DECLARE(seven_segments_64);
LV_FONT_DECLARE(font_vi_20);

static lv_obj_t *hour_label = NULL;
static lv_obj_t *min_label = NULL;
static lv_obj_t *colon_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *weekday_rects[7] = {NULL};
static lv_timer_t *update_timer = NULL;
static lv_timer_t *notification_timer = NULL;
static lv_obj_t *notification_box = NULL;
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

static void hide_notification_cb(lv_timer_t *timer) {
  LV_UNUSED(timer);
  LOG_INF("Hiding notification box");
  if (notification_box) {
    lv_obj_del(notification_box);
    notification_box = NULL;
    LOG_DBG("Notification box deleted");
  }
  if (notification_timer) {
    notification_timer = NULL;
  }
}

static void segments_wf_app_deinit(void) {
  LOG_INF("Segments watchface app deinit");
  if (update_timer) {
    lv_timer_del(update_timer);
    update_timer = NULL;
  }
  if (notification_timer) {
    lv_timer_del(notification_timer);
    notification_timer = NULL;
  }
  if (notification_box) {
    lv_obj_del(notification_box);
    notification_box = NULL;
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
  if (!ev) {
    return;
  }

  // Handle new notification event
  if (ev->type == INPUT_EVENT_TYPE_NOTIFICATION &&
      ev->code == INPUT_NOTIFICATION_NEW) {
    LOG_INF("New notification received");

    const struct ancs_notification *notif =
        (const struct ancs_notification *)ev->data;
    if (!notif) {
      LOG_WRN("Notification data is NULL");
      return;
    }

    LOG_INF("Notification title: %s", notif->title ? notif->title : "(null)");
    LOG_INF("Notification message: %s", notif->message ? notif->message : "(null)");

    // Remove existing notification if any
    if (notification_timer) {
      lv_timer_del(notification_timer);
      notification_timer = NULL;
      LOG_DBG("Deleted existing notification timer");
    }
    if (notification_box) {
      lv_obj_del(notification_box);
      notification_box = NULL;
      LOG_DBG("Deleted existing notification box");
    }

    // Create notification overlay box
    // Get display width and calculate box width (display_width - 40 for 20px margins)
    lv_disp_t *display = lv_disp_get_default();
    int32_t display_width = lv_disp_get_hor_res(display);
    int32_t box_width = display_width - 40;

    LOG_INF("Creating notification box: width=%d, height=120", box_width);

    notification_box = lv_obj_create(lv_scr_act());
    lv_obj_set_size(notification_box, box_width, 120);
    lv_obj_align(notification_box, LV_ALIGN_CENTER, 0, 0);
    
    // Move notification box to foreground to ensure it's on top
    lv_obj_move_foreground(notification_box);

    // Style: rounded rectangle with white background and black border
    static lv_style_t notification_style;
    lv_style_init(&notification_style);
    lv_style_set_bg_color(&notification_style, lv_color_white());
    lv_style_set_bg_opa(&notification_style, LV_OPA_COVER);
    lv_style_set_radius(&notification_style, 10);
    lv_style_set_border_width(&notification_style, 2);
    lv_style_set_border_color(&notification_style, lv_color_black());
    lv_style_set_pad_all(&notification_style, 12);
    lv_obj_add_style(notification_box, &notification_style, 0);

    // Create label for notification text
    lv_obj_t *noti_label = lv_label_create(notification_box);
    lv_label_set_long_mode(noti_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(noti_label, box_width - 24);
    lv_obj_align(noti_label, LV_ALIGN_CENTER, 0, 0);

    // Display notification content
    if (notif->title && notif->message) {
      lv_label_set_text_fmt(noti_label, "%s\n%s", notif->title, notif->message);
    } else if (notif->title) {
      lv_label_set_text(noti_label, notif->title);
    } else if (notif->message) {
      lv_label_set_text(noti_label, notif->message);
    } else {
      lv_label_set_text(noti_label, "New Notification");
    }

    LOG_INF("Notification label created with text");

    // Style text label - black text with font_vi_20
    static lv_style_t noti_text_style;
    lv_style_init(&noti_text_style);
    lv_style_set_text_color(&noti_text_style, lv_color_black());
    lv_style_set_text_align(&noti_text_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_font(&noti_text_style, &font_vi_20);
    lv_obj_add_style(noti_label, &noti_text_style, 0);

    // Mark the screen as dirty to force a redraw
    lv_obj_invalidate(lv_scr_act());
    
    LOG_INF("Notification box displayed, will hide in 10 seconds");

    // Set timer to hide notification after 10 seconds
    notification_timer = lv_timer_create(hide_notification_cb, 10000, NULL);
  }
}

IApp SegmentsWatchfaceApp = {
    .init = segments_wf_app_init,
    .deinit = segments_wf_app_deinit,
    .handle_event = segments_wf_app_handle_event,
};
