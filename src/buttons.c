/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NOTE: If you are looking into an implementation of button events with
 * debouncing, check out `input` subsystem and `samples/subsys/input/input_dump`
 * example instead.
 */

#include <inttypes.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include "app/gpio_event.h"

LOG_MODULE_REGISTER(buttons, LOG_LEVEL_INF);

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#define SW1_NODE DT_ALIAS(sw1)
#define SW2_NODE DT_ALIAS(sw2)
#define SW3_NODE DT_ALIAS(sw3)

#define MAX_BUTTONS 4
static struct gpio_callback button_cb_data[MAX_BUTTONS];

static const struct gpio_dt_spec buttons[MAX_BUTTONS] = {
    GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(SW2_NODE, gpios, {0}),
    GPIO_DT_SPEC_GET_OR(SW3_NODE, gpios, {0}),
};

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
  LOG_INF("Button event port = %s pins = 0x%08x", dev->name, pins);

  // Find which button triggered the interrupt and check its state
  for (int i = 0; i < MAX_BUTTONS; i++) {
    const struct gpio_dt_spec *button = &buttons[i];
    if (button->port == dev && (pins & BIT(button->pin))) {
      // Read actual button state
      int val = gpio_pin_get_dt(button);
      if (val < 0) {
        LOG_ERR("Failed to read button state: %d", val);
        continue;
      }

      // Send event based on actual state (pressed=1, released=0)
      gpio_button_callback_mapped(button->pin, val == 1);
      LOG_INF("Button %d %s", i, val == 1 ? "pressed" : "released");
    }
  }
}

int button_init(void) {
  int ret;
  for (int i = 0; i < MAX_BUTTONS; i++) {
    const struct gpio_dt_spec *button = &buttons[i];
    if (button->port == NULL) {
      LOG_WRN("Button %d not defined in device tree, skipping...", i);
      continue;
    }

    if (!gpio_is_ready_dt(button)) {
      LOG_ERR("Error: button device %s is not ready\n", button->port->name);
      continue;
    }

    ret = gpio_pin_configure_dt(button, GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("Error %d: failed to configure %s pin %d\n", ret, button->port->name, button->pin);
      continue;
    }

    ret = gpio_pin_interrupt_configure_dt(button, GPIO_INT_EDGE_BOTH);
    if (ret != 0) {
      LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n", ret, button->port->name,
              button->pin);
      continue;
    }

    gpio_init_callback(&button_cb_data[i], button_pressed, BIT(button->pin));
    gpio_add_callback(button->port, &button_cb_data[i]);
    LOG_INF("Set up button at %s pin %d\n", button->port->name, button->pin);
  }
  return 0;
}
