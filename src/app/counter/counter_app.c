/**
 * @file counter_app.c
 * @brief Example counter application
 *
 * Demonstrates the IApp interface with a simple counter that
 * increments on button press
 */

#include <zephyr/logging/log.h>

#include "../app_interface.h"
#include "lvgl.h"

LOG_MODULE_REGISTER(counter_app, LOG_LEVEL_INF);

static int counter = 0;
static lv_obj_t *static_label = NULL;
static lv_obj_t *dynamic_label = NULL;

/**
 * @brief Initialize the counter app UI
 */
static void counter_app_init(void) {
  LOG_INF("Counter app init");

  // Clean the screen
  lv_obj_clean(lv_scr_act());

  // Create static label for "Counter:"
  static_label = lv_label_create(lv_scr_act());
  lv_label_set_text(static_label, "Counter:");
  lv_obj_align(static_label, LV_ALIGN_CENTER, 0, -15);

  // Create dynamic label for the number
  dynamic_label = lv_label_create(lv_scr_act());
  lv_label_set_text_fmt(dynamic_label, "%d", counter);
  lv_obj_align(dynamic_label, LV_ALIGN_CENTER, 0, 15);

  // Optional: Style the labels
  static lv_style_t style;
  lv_style_init(&style);
  lv_style_set_text_font(&style, &lv_font_montserrat_24);
  lv_obj_add_style(static_label, &style, 0);
  lv_obj_add_style(dynamic_label, &style, 0);
}

/**
 * @brief Cleanup the counter app UI
 */
static void counter_app_deinit(void) {
  LOG_INF("Counter app deinit");
  lv_obj_clean(lv_scr_act());
  static_label = NULL;
  dynamic_label = NULL;
}

/**
 * @brief Handle input events for counter app
 *
 * @param ev Input event
 */
static void counter_app_handle_event(input_event_t *ev) {
  if (ev == NULL || dynamic_label == NULL) {
    return;
  }

  // Handle button press events
  if (ev->type == INPUT_EVENT_TYPE_KEY && ev->value == 1) {
    switch (ev->code) {
    case INPUT_KEY_ENTER:
      // Increment counter on ENTER press
      counter++;
      LOG_INF("Counter incremented to %d", counter);
      lv_label_set_text_fmt(dynamic_label, "%d", counter);
      break;

    case INPUT_KEY_BACK:
      // Reset counter on BACK press
      counter = 0;
      LOG_INF("Counter reset");
      lv_label_set_text_fmt(dynamic_label, "%d", counter);
      break;

    case INPUT_KEY_UP:
      // Increment by 10
      counter += 10;
      LOG_INF("Counter incremented by 10 to %d", counter);
      lv_label_set_text_fmt(dynamic_label, "%d", counter);
      break;

    case INPUT_KEY_DOWN:
      // Decrement by 10
      counter -= 10;
      LOG_INF("Counter decremented by 10 to %d", counter);
      lv_label_set_text_fmt(dynamic_label, "%d", counter);
      break;
    }
  }
}

/**
 * @brief Counter app instance
 */
IApp CounterApp = {
    .init = counter_app_init,
    .deinit = counter_app_deinit,
    .handle_event = counter_app_handle_event,
};
