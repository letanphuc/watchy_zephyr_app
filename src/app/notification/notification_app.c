/**
 * @file notification_app.c
 * @brief Simple notification preview app
 */

#include <zephyr/logging/log.h>

#include "../app_interface.h"
#include "lvgl.h"

LOG_MODULE_REGISTER(notification_app, LOG_LEVEL_INF);

#if defined(HAS_FONT_VI_20)
LV_FONT_DECLARE(font_vi_20);
#define NOTIFICATION_FONT &font_vi_20
#else
#define NOTIFICATION_FONT LV_FONT_DEFAULT
#endif

static lv_obj_t *noti_label = NULL;

static void notification_app_init(void) {
  LOG_INF("Notification app init");

  lv_obj_clean(lv_scr_act());

  noti_label = lv_label_create(lv_scr_act());
  lv_label_set_text(noti_label, "Xin chào");
  lv_obj_set_style_text_font(noti_label, NOTIFICATION_FONT, 0);
  lv_obj_set_style_text_align(noti_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(noti_label, LV_ALIGN_CENTER, 0, 0);
}

static void notification_app_deinit(void) {
  LOG_INF("Notification app deinit");
  lv_obj_clean(lv_scr_act());
  noti_label = NULL;
}

static void notification_app_handle_event(input_event_t *ev) { (void)ev; }

IApp NotificationApp = {
    .init = notification_app_init,
    .deinit = notification_app_deinit,
    .handle_event = notification_app_handle_event,
};
