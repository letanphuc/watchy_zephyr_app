/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "buttons.h"

extern int sensor_init(void);
extern void epd_display_init(void);
extern int display_main(void);

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
  button_init();
  // sensor_init();
  // epd_display_init();
  display_main();

  return 0;
}
