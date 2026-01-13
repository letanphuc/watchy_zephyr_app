#pragma once

#include <zephyr/drivers/rtc.h>

int set_date_time(const struct device* rtc);
int get_date_time(const struct device* rtc);
