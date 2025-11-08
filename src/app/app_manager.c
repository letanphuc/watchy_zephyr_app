/**
 * @file app_manager.c
 * @brief Application Manager implementation
 */

#include "app_manager.h"
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_manager, LOG_LEVEL_INF);

#define MAX_APPS 8

/**
 * @brief Application Manager state
 */
typedef struct {
  IApp *apps[MAX_APPS]; /**< Array of registered apps */
  IApp *active;         /**< Currently active app */
  uint8_t count;        /**< Number of registered apps */
  uint8_t active_index; /**< Index of active app */
} AppManager;

static AppManager manager = {.apps = {NULL}, .active = NULL, .count = 0, .active_index = 0xFF};

void app_manager_register(IApp *app) {
  if (app == NULL) {
    LOG_ERR("Cannot register NULL app");
    return;
  }

  if (manager.count >= MAX_APPS) {
    LOG_ERR("Maximum number of apps reached");
    return;
  }

  manager.apps[manager.count] = app;
  LOG_INF("Registered app at index %d", manager.count);
  manager.count++;
}

void app_manager_launch(uint8_t index) {
  if (index >= manager.count) {
    LOG_ERR("Invalid app index: %d", index);
    return;
  }

  IApp *new_app = manager.apps[index];
  if (new_app == NULL) {
    LOG_ERR("App at index %d is NULL", index);
    return;
  }

  // Deinitialize current app
  if (manager.active != NULL && manager.active->deinit != NULL) {
    LOG_INF("Deinitializing app at index %d", manager.active_index);
    manager.active->deinit();
  }

  // Initialize new app
  manager.active = new_app;
  manager.active_index = index;

  if (manager.active->init != NULL) {
    LOG_INF("Initializing app at index %d", index);
    manager.active->init();
  }
}

void app_manager_handle_event(input_event_t *ev) {
  if (ev == NULL) {
    return;
  }

  if (manager.active != NULL && manager.active->handle_event != NULL) {
    manager.active->handle_event(ev);
  }
}

uint8_t app_manager_get_count(void) { return manager.count; }

uint8_t app_manager_get_active_index(void) { return manager.active_index; }

void app_manager_switch_next(void) {
  if (manager.count == 0) {
    LOG_WRN("No apps registered");
    return;
  }

  uint8_t next_index = (manager.active_index + 1) % manager.count;
  LOG_INF("Switching to next app: %d -> %d", manager.active_index, next_index);
  app_manager_launch(next_index);
}
