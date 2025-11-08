/**
 * @file images_app.c
 * @brief Image viewer application
 *
 * Demonstrates switching between two images using button presses
 */

#include <zephyr/logging/log.h>

#include "../app_interface.h"
#include "lvgl.h"

LOG_MODULE_REGISTER(images_app, LOG_LEVEL_INF);

// External image declarations
extern const lv_image_dsc_t img1;
extern const lv_image_dsc_t img2;

static int current_image = 0; // 0 = img1, 1 = img2
static lv_obj_t *image_widget = NULL;

/**
 * @brief Initialize the image viewer app UI
 */
static void images_app_init(void) {
  LOG_INF("Image viewer app init");

  // Clean the screen
  lv_obj_clean(lv_scr_act());

  // Create image widget
  image_widget = lv_image_create(lv_scr_act());

  // Set initial image
  lv_image_set_src(image_widget, &img1);
  lv_obj_set_style_transform_scale(image_widget, 256, LV_PART_MAIN);

  // Center the image
  lv_obj_align(image_widget, LV_ALIGN_CENTER, 0, 0);

  current_image = 0;
}

/**
 * @brief Cleanup the image viewer app UI
 */
static void images_app_deinit(void) {
  LOG_INF("Image viewer app deinit");
  lv_obj_clean(lv_scr_act());
  image_widget = NULL;
}

/**
 * @brief Handle input events for image viewer app
 *
 * @param ev Input event
 */
static void images_app_handle_event(input_event_t *ev) {
  if (ev == NULL || image_widget == NULL) {
    return;
  }

  // Handle button press events
  if (ev->type == INPUT_EVENT_TYPE_KEY && ev->value == 1) {
    switch (ev->code) {
    case INPUT_KEY_ENTER:
    case INPUT_KEY_UP:
    case INPUT_KEY_DOWN:
    case INPUT_KEY_BACK:
      // Toggle between images on any button press
      current_image = 1 - current_image;

      if (current_image == 0) {
        lv_image_set_src(image_widget, &img1);
        LOG_INF("Switched to image 1");
      } else {
        lv_image_set_src(image_widget, &img2);
        LOG_INF("Switched to image 2");
      }
      break;
    }
  }
}

/**
 * @brief Image viewer app instance
 */
IApp ImagesApp = {
    .init = images_app_init,
    .deinit = images_app_deinit,
    .handle_event = images_app_handle_event,
};
