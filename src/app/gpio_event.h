/**
 * @file gpio_event.h
 * @brief GPIO event dispatcher header
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Enhanced GPIO button callback with key mapping
 *
 * @param gpio_pin GPIO pin number
 * @param pressed true if pressed, false if released
 */
void gpio_button_callback_mapped(uint8_t gpio_pin, bool pressed);

/**
 * @brief Map GPIO pin to logical key code
 *
 * @param gpio_pin GPIO pin number
 * @return Logical key code
 */
uint8_t gpio_pin_to_key(uint8_t gpio_pin);
