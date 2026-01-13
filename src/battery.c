#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(battery_app, LOG_LEVEL_DBG);

#define BATT_ADC_CH DT_ALIAS(batt_adc_channel)
static const struct device *batt_adc = DEVICE_DT_GET(DT_ALIAS(batt_adc));
static const struct adc_channel_cfg batt_adc_ch =
    ADC_CHANNEL_CFG_DT(BATT_ADC_CH);

static int32_t batt_vref_mv;
static uint8_t batt_resolution;

void battery_init(void) {
  batt_vref_mv = DT_PROP(BATT_ADC_CH, zephyr_vref_mv);
  batt_resolution = DT_PROP(BATT_ADC_CH, zephyr_resolution);

  if (!device_is_ready(batt_adc)) {
    LOG_ERR("Battery ADC not ready");
    return;
  }
  if (adc_channel_setup(batt_adc, &batt_adc_ch) < 0) {
    LOG_ERR("Failed to setup battery ADC channel");
    return;
  }
}

int32_t read_batt_voltage(void) {
  uint16_t buf;
  struct adc_sequence seq = {.channels = BIT(batt_adc_ch.channel_id),
                             .buffer = &buf,
                             .buffer_size = sizeof(buf),
                             .resolution = batt_resolution};

  // dump seq content for debugging
  LOG_DBG("ADC Sequence - channels: 0x%08x, buffer_size: %d, resolution: %d",
          seq.channels, seq.buffer_size, seq.resolution);

  if (adc_read(batt_adc, &seq) < 0) {
    LOG_ERR("Failed to read battery voltage");
    return -1;
  }

  LOG_DBG("Raw ADC Value: %d", buf);
  return (int32_t)buf * batt_vref_mv / (1 << batt_resolution);
}

void battery_main(void) {
  battery_init();
  while (1) {
    int32_t voltage = read_batt_voltage();
    if (voltage >= 0) {
      LOG_INF("Battery Voltage: %d mV", voltage);
    }
    k_sleep(K_SECONDS(2));
  }
}
