/**
 * @file app_interface.h
 * @brief Base interface for LVGL applications
 */

#pragma once

#include <stdint.h>

/**
 * @brief Input event structure for GPIO and other input events
 */
typedef struct {
  uint8_t type;  /**< Event type (e.g., 1 = KEY) */
  uint8_t code;  /**< Event code (e.g., GPIO pin or key code) */
  int32_t value; /**< Event value (e.g., 0 = release, 1 = press) */
  void *data;    /**< Event data pointer (e.g., pointer to notification) */
} input_event_t;

/**
 * @brief Application interface
 *
 * Each app must implement this interface to be managed by the AppManager
 */
typedef struct IApp {
  void (*init)(void);   /**< Initialize app and create UI */
  void (*deinit)(void); /**< Cleanup app and destroy UI */
  void (*handle_event)(input_event_t *event); /**< Handle input events */
} IApp;

/**
 * @brief Event type definitions
 */
#define INPUT_EVENT_TYPE_KEY 1
#define INPUT_EVENT_TYPE_TOUCH 2
#define INPUT_EVENT_TYPE_NOTIFICATION 3

/**
 * @brief Common key codes (mapped to GPIO pins)
 */
#define INPUT_KEY_BACK 0
#define INPUT_KEY_UP 1
#define INPUT_KEY_DOWN 2
#define INPUT_KEY_ENTER 3

/**
 * @brief Notification codes
 */
#define INPUT_NOTIFICATION_NEW 1
