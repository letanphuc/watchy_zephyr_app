/**
 * @file gpio_event.c
 * @brief GPIO event dispatcher - bridges GPIO callbacks to app events
 */

#include "app_interface.h"
#include "app_manager.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_event, LOG_LEVEL_INF);

/**
 * @brief Map GPIO pin to logical key code
 *
 * Maps physical GPIO pins to logical key codes based on your
 * hardware configuration.
 *
 * @param gpio_pin GPIO pin number
 * @return Logical key code
 */
uint8_t gpio_pin_to_key(uint8_t gpio_pin) {
  // Adjust these mappings based on your actual GPIO configuration
  // This is just an example mapping
  switch (gpio_pin) {
  case 26:
    return INPUT_KEY_BACK; // SW0
  case 25:
    return INPUT_KEY_UP; // SW1
  case 32:
    return INPUT_KEY_DOWN; // SW2
  case 4:
    return INPUT_KEY_ENTER; // SW3
  default:
    return gpio_pin;
  }
}

/**
 * @brief Enhanced GPIO button callback with key mapping
 *
 * @param gpio_pin GPIO pin number
 * @param pressed true if pressed, false if released
 */
void gpio_button_callback_mapped(uint8_t gpio_pin, bool pressed) {
  // Button 0 (pin 26, SW0) is reserved for app switching
  // Only trigger on button press (not release)
  if (gpio_pin == 26 && pressed) {
    LOG_INF("App switch button pressed");
    app_manager_switch_next();
    return;
  }

  uint8_t key_code = gpio_pin_to_key(gpio_pin);

  input_event_t ev = {.type = INPUT_EVENT_TYPE_KEY, .code = key_code, .value = pressed ? 1 : 0};

  LOG_INF("Key event: code=%d, pressed=%d", key_code, pressed);
  app_manager_handle_event(&ev);
}
