/**
 * @file app_manager.h
 * @brief Application Manager for handling app lifecycle and events
 */

#pragma once

#include "app_interface.h"
#include <stdint.h>

/**
 * @brief Register an application with the AppManager
 *
 * @param app Pointer to IApp structure
 */
void app_manager_register(IApp *app);

/**
 * @brief Launch an application by index
 *
 * Stops the currently running app and launches the new one
 *
 * @param index Index of the app in the registered apps array
 */
void app_manager_launch(uint8_t index);

/**
 * @brief Handle input event and forward to active app
 *
 * @param ev Pointer to input event
 */
void app_manager_handle_event(input_event_t *ev);

/**
 * @brief Get the number of registered apps
 *
 * @return Number of apps registered
 */
uint8_t app_manager_get_count(void);

/**
 * @brief Get the currently active app index
 *
 * @return Active app index or 0xFF if none active
 */
uint8_t app_manager_get_active_index(void);

/**
 * @brief Switch to the next app in the registered apps list
 *
 * Cycles through registered apps (wraps around to 0 after last app)
 */
void app_manager_switch_next(void);
