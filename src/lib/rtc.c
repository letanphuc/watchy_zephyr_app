#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "rtc.h"
LOG_MODULE_REGISTER(rtc_app, 4);

const struct device* const rtc = DEVICE_DT_GET(DT_ALIAS(rtc));

int set_date_time(const struct device* rtc) {
  int ret = 0;
  struct rtc_time tm = {
      .tm_year = 2024 - 1900,
      .tm_mon = 11 - 1,
      .tm_mday = 17,
      .tm_hour = 4,
      .tm_min = 19,
      .tm_sec = 0,
  };

  ret = rtc_set_time(rtc, &tm);
  if (ret < 0) {
    printk("Cannot write date time: %d\n", ret);
    return ret;
  }
  return ret;
}

int get_date_time(const struct device* rtc) {
  int ret = 0;
  struct rtc_time tm;

  ret = rtc_get_time(rtc, &tm);
  if (ret < 0) {
    printk("Cannot read date time: %d\n", ret);
    return ret;
  }

  printk("RTC date and time: %04d-%02d-%02d %02d:%02d:%02d\n",
         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
         tm.tm_sec);

  return ret;
}

int rtc_main(void) {
  /* Check if the RTC is ready */
  if (!device_is_ready(rtc)) {
    printk("Device is not ready\n");
    return 0;
  }

  set_date_time(rtc);

  while (1) {
    /* code */
    get_date_time(rtc);
    k_sleep(K_MSEC(1000));
  }

  return 0;
}
