/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

#include "buttons.h"

extern int sensor_init(void);

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void) {
  button_init();
  sensor_init();

  return 0;
}
