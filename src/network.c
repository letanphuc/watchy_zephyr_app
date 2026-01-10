#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

LOG_MODULE_REGISTER(network, LOG_LEVEL_INF);

#define WIFI_SAMPLE_SSID "mystery 2.4"
#define WIFI_SAMPLE_PSK "123456Aa@"

#define NET_EVENT_WIFI_MASK                                           \
  (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT | \
   NET_EVENT_WIFI_IFACE_STATUS)

static struct net_if *sta_iface;
static struct wifi_connect_req_params sta_config;
static struct net_mgmt_event_callback cb;

/* Check necessary definitions */
BUILD_ASSERT(sizeof(WIFI_SAMPLE_SSID) > 1,
             "WIFI_SAMPLE_SSID is empty. Please set it in conf file.");

static int connect_to_wifi(void);

static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint64_t mgmt_event, struct net_if *iface) {
  switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT: {
      LOG_INF("Connected to %s", WIFI_SAMPLE_SSID);
      break;
    }
    case NET_EVENT_WIFI_DISCONNECT_RESULT: {
      LOG_INF("Disconnected from %s", WIFI_SAMPLE_SSID);
      //   Attempt to reconnect
      connect_to_wifi();
      break;
    }
    default:
      LOG_INF("Unhandled WiFi event: %llu", mgmt_event);
      break;
  }
}

static int connect_to_wifi(void) {
  if (!sta_iface) {
    LOG_ERR("STA: interface not initialized");
    return -EIO;
  }

  sta_config.ssid = (const uint8_t *)WIFI_SAMPLE_SSID;
  sta_config.ssid_length = sizeof(WIFI_SAMPLE_SSID) - 1;
  sta_config.psk = (const uint8_t *)WIFI_SAMPLE_PSK;
  sta_config.psk_length = sizeof(WIFI_SAMPLE_PSK) - 1;
  sta_config.security = WIFI_SECURITY_TYPE_PSK;
  sta_config.channel = WIFI_CHANNEL_ANY;
  sta_config.band = WIFI_FREQ_BAND_2_4_GHZ;

  LOG_INF("Connecting to SSID: %s", sta_config.ssid);

  int ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, sta_iface, &sta_config,
                     sizeof(struct wifi_connect_req_params));
  if (ret) {
    LOG_ERR("Unable to Connect to (%s), err: %d", WIFI_SAMPLE_SSID, ret);
  }

  return ret;
}

int init_net(void) {
  net_mgmt_init_event_callback(&cb, wifi_event_handler, NET_EVENT_WIFI_MASK);
  net_mgmt_add_event_callback(&cb);

  /* Get STA interface */
  sta_iface = net_if_get_wifi_sta();
  if (!sta_iface) {
    LOG_ERR("Failed to get WiFi STA interface");
    return -ENODEV;
  }

  int ret = connect_to_wifi();
  return ret;
}
