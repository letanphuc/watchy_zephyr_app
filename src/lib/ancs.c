#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "ancs.h"
#include "host/gatt_internal.h"
#include "host/hci_core.h"

#ifndef CONFIG_ANCS_LOG_LEVEL
#define CONFIG_ANCS_LOG_LEVEL LOG_LEVEL_INF
#endif

#ifndef CONFIG_ANCS_WORK_QUEUE_STACK_SIZE
#define CONFIG_ANCS_WORK_QUEUE_STACK_SIZE 512
#endif

#ifndef CONFIG_ANCS_WORK_QUEUE_PRIORITY
#define CONFIG_ANCS_WORK_QUEUE_PRIORITY 7
#endif

#ifndef CONFIG_ANCS_NOTIFICATION_POOL_SIZE
#define CONFIG_ANCS_NOTIFICATION_POOL_SIZE 2
#endif

#ifndef CONFIG_ANCS_NOTIFICATION_QUEUE_SIZE
#define CONFIG_ANCS_NOTIFICATION_QUEUE_SIZE 2
#endif

#ifndef CONFIG_ANCS_DATA_SOURCE_BUFFER_SIZE
#define CONFIG_ANCS_DATA_SOURCE_BUFFER_SIZE 512
#endif

/* Kconfig should be used for these */
#define NOTIFICATION_POOL_SIZE CONFIG_ANCS_NOTIFICATION_POOL_SIZE
#define NOTIFICATION_QUEUE_SIZE CONFIG_ANCS_NOTIFICATION_QUEUE_SIZE
#define DATA_SOURCE_BUFFER_SIZE CONFIG_ANCS_DATA_SOURCE_BUFFER_SIZE

LOG_MODULE_REGISTER(ancs, CONFIG_ANCS_LOG_LEVEL);

#define LOG_ERR_OR_DBG(err, ...) \
  do {                           \
    if (err) {                   \
      LOG_ERR(__VA_ARGS__);      \
    } else {                     \
      LOG_DBG(__VA_ARGS__);      \
    }                            \
  } while (0)

/* ANCS Service and Characteristic UUIDs */
static struct bt_uuid_128 ancs_service_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x7905F431, 0xB5CE, 0x4E99, 0xA40F, 0x4B1E122D00D0));
static struct bt_uuid_128 notif_source_char_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x9FBF120D, 0x6301, 0x42D9, 0x8C58, 0x25E699A21DBD));
static struct bt_uuid_128 control_point_char_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x69D1D8F3, 0x45E1, 0x49A8, 0x9821, 0x9BBDFDAAD9D9));
static struct bt_uuid_128 data_source_char_uuid = BT_UUID_INIT_128(
    BT_UUID_128_ENCODE(0x22EAC6E9, 0x24D6, 0x4BB5, 0xBE44, 0xB36ACE7C7BFB));

/* Internal ANCS state representation */
enum {
  ANCS_STATE_IDLE,
  ANCS_STATE_DISCOVERING,
  ANCS_STATE_START_SUBSCRIPTIONS,
  ANCS_STATE_SUBSCRIBING_NS,
  ANCS_STATE_SUBSCRIBING_DS,
  ANCS_STATE_ENABLED,
};

/* Internal Command and Attribute IDs */
typedef enum {
  COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES = 0,
  COMMAND_ID_GET_APP_ATTRIBUTES = 1,
  COMMAND_ID_PERFORM_NOTIFICATION_ACTION = 2,
} command_id_t;

typedef enum {
  ATTR_ID_APP_IDENTIFIER = 0,
  ATTR_ID_TITLE = 1,
  ATTR_ID_SUBTITLE = 2,
  ATTR_ID_MESSAGE = 3,
  ATTR_ID_MESSAGE_SIZE = 4,
  ATTR_ID_DATE = 5,
  ATTR_ID_POSITIVE_ACTION_LABEL = 6,
  ATTR_ID_NEGATIVE_ACTION_LABEL = 7,
} notification_attribute_id_t;

/* Static data for the ANCS client */
static struct {
  struct bt_conn *conn;
  uint8_t state;
  uint16_t ns_handle;
  uint16_t cp_handle;
  uint16_t ds_handle;
  struct bt_gatt_subscribe_params ns_sub_params;
  struct bt_gatt_subscribe_params ds_sub_params;
  struct bt_gatt_discover_params discover_params;
  const struct ancs_callbacks *app_cb;

  /* Pool to store notifications being processed */
  struct ancs_notification notif_pool[NOTIFICATION_POOL_SIZE];
  bool pool_in_use[NOTIFICATION_POOL_SIZE];

  /* Data source parsing state */
  uint8_t ds_buffer[DATA_SOURCE_BUFFER_SIZE];
  size_t ds_buffer_len;
  int current_notif_idx;  // Index in notif_pool
  int num_attr_requested;
  int remain_num_attr;
  bool is_init;

} ancs;

/* Worker thread to request notification attributes */
static struct k_work_q ancs_work_q;
static K_KERNEL_STACK_DEFINE(ancs_stack, CONFIG_ANCS_WORK_QUEUE_STACK_SIZE);

K_MSGQ_DEFINE(notification_q, sizeof(uint32_t), NOTIFICATION_QUEUE_SIZE, 4);
static void req_notif_info_work_handler(struct k_work *work);
static struct k_work req_notif_info_work;

static void discovery_work_handler(struct k_work *work);
static struct k_work_delayable discovery_work;

K_MUTEX_DEFINE(ancs_pool_mutex);
K_SEM_DEFINE(ancs_data_sem, 0, 1);

/* Forward declarations */
static void ancs_reset_state(void);
static int subscribe_to_ns(struct bt_conn *conn);
static int subscribe_to_ds(struct bt_conn *conn);
static void process_notification_attributes(const uint8_t *data,
                                            uint16_t length);

/*** Notification Pool Management ***/

static int find_free_pool_slot(void) {
  for (int i = 0; i < NOTIFICATION_POOL_SIZE; i++) {
    if (!ancs.pool_in_use[i]) {
      return i;
    }
  }
  return -1;
}

static int find_pool_slot_by_uid(uint32_t uid) {
  for (int i = 0; i < NOTIFICATION_POOL_SIZE; i++) {
    if (ancs.pool_in_use[i] &&
        ancs.notif_pool[i].source.notification_uid == uid) {
      return i;
    }
  }
  return -1;
}

/*** GATT Notification Handlers ***/

static uint8_t data_source_notify_cb(struct bt_conn *conn,
                                     struct bt_gatt_subscribe_params *params,
                                     const void *data, uint16_t length) {
  if (!data || length == 0) {
    return BT_GATT_ITER_STOP;
  }
  LOG_HEXDUMP_DBG(data, length, "Data Source Data");
  process_notification_attributes(data, length);
  return BT_GATT_ITER_CONTINUE;
}

static uint8_t notif_source_notify_cb(struct bt_conn *conn,
                                      struct bt_gatt_subscribe_params *params,
                                      const void *data, uint16_t length) {
  if (!data || length < 8 || ancs.state != ANCS_STATE_ENABLED) {
    return BT_GATT_ITER_STOP;
  }

  const uint8_t *raw = data;
  struct ancs_notification_source src;

  src.event_id = raw[0];
  src.event_flags = raw[1];
  src.category_id = raw[2];
  src.category_count = raw[3];
  src.notification_uid = sys_get_le32(&raw[4]);

  LOG_DBG("NS: Event=%d, Flags=%d, Cat=%d, Count=%d, UID=0x%x", src.event_id,
          src.event_flags, src.category_id, src.category_count,
          src.notification_uid);

  if (src.event_id == ANCS_EVENT_ID_NOTIFICATION_ADDED ||
      src.event_id == ANCS_EVENT_ID_NOTIFICATION_MODIFIED) {
    k_mutex_lock(&ancs_pool_mutex, K_FOREVER);
    int idx = find_pool_slot_by_uid(src.notification_uid);
    if (idx < 0) {
      idx = find_free_pool_slot();
    }

    if (idx >= 0) {
      ancs.notif_pool[idx].source = src;
      ancs.pool_in_use[idx] = true;
      k_mutex_unlock(&ancs_pool_mutex);

      if (k_msgq_put(&notification_q, &src.notification_uid, K_NO_WAIT) != 0) {
        uint32_t used = k_msgq_num_used_get(&notification_q);

        LOG_WRN("Notification queue full (%u/%u used), dropping UID 0x%x", used,
                (uint32_t)NOTIFICATION_QUEUE_SIZE, src.notification_uid);
      }
    } else {
      k_mutex_unlock(&ancs_pool_mutex);
      LOG_WRN("Notification pool full, dropping notification");
    }

    if (ancs.state == ANCS_STATE_ENABLED) {
      k_work_submit_to_queue(&ancs_work_q, &req_notif_info_work);
    }

  } else if (src.event_id == ANCS_EVENT_ID_NOTIFICATION_REMOVED) {
    LOG_DBG("ANCS notification removed: %d", src.notification_uid);
    // Here we just notify the app. We don't need to clear from the pool
    // as it will be cleared after a potential ongoing attribute fetch
    // completes.
    if (ancs.app_cb && ancs.app_cb->on_notification_removed) {
      ancs.app_cb->on_notification_removed(src.notification_uid);
    }
  }

  return BT_GATT_ITER_CONTINUE;
}

/*** GATT Discovery and Subscription Logic ***/

static uint8_t discover_func(struct bt_conn *conn,
                             const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params) {
  // This is the completion callback, called when discovery finishes or an error
  // occurs.
  if (!attr) {
    LOG_WRN("Discovery complete, not all required ANCS handles were found.");
    ancs_reset_state();
    return BT_GATT_ITER_STOP;
  }

  // --- Step 1: Discover the ANCS Primary Service ---
  if (params->type == BT_GATT_DISCOVER_PRIMARY) {
    // This callback should only be for our service UUID.
    LOG_INF("ANCS Primary Service found, handle 0x%04x.", attr->handle);

    // Now, start discovering the characteristics within this service.
    params->uuid =
        NULL;  // We want to find all characteristics, not a specific one.
    params->start_handle = attr->handle + 1;
    params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

    if (bt_gatt_discover(conn, params) != 0) {
      LOG_ERR("Characteristic discovery failed.");
      ancs_reset_state();
    }

    // Return STOP because we are starting a new discovery from within the
    // callback.
    return BT_GATT_ITER_STOP;
  }

  // --- Step 2: Discover the Characteristics ---
  // At this point, params->type is BT_GATT_DISCOVER_CHARACTERISTIC
  // The characteristic's properties are in attr->user_data.
  struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;
  if (bt_uuid_cmp(chrc->uuid, &notif_source_char_uuid.uuid) == 0) {
    LOG_INF("Found Notification Source characteristic, handle 0x%04x",
            chrc->value_handle);
    ancs.ns_handle = chrc->value_handle;
  } else if (bt_uuid_cmp(chrc->uuid, &control_point_char_uuid.uuid) == 0) {
    LOG_INF("Found Control Point characteristic, handle 0x%04x",
            chrc->value_handle);
    ancs.cp_handle = chrc->value_handle;
  } else if (bt_uuid_cmp(chrc->uuid, &data_source_char_uuid.uuid) == 0) {
    LOG_INF("Found Data Source characteristic, handle 0x%04x",
            chrc->value_handle);
    ancs.ds_handle = chrc->value_handle;
  }

  // Check if we have found all the characteristics we need.
  if (ancs.ns_handle && ancs.cp_handle && ancs.ds_handle) {
    LOG_INF("All required ANCS characteristics found.");
    ancs.state = ANCS_STATE_START_SUBSCRIPTIONS;
    // Check if device is bonded before subscribing
    struct bt_conn_info info = {0};
    bt_conn_get_info(conn, &info);
    if (bt_le_bond_exists(info.id, info.le.dst)) {
      if (bt_conn_get_security(conn) < BT_SECURITY_L2) {
        // Clear the bond and restart pairing
        LOG_WRN("Connection not encrypted, removing bond to restart pairing.");
        bt_unpair(info.id, info.le.dst);
      } else {
        LOG_INF(
            "Device is bonded and connection is secure, subscribing to NS and "
            "DS.");
        subscribe_to_ds(conn);
      }
    } else {
      LOG_INF("Device not bonded, request pairing now.");
      int err = bt_conn_set_security(conn, BT_SECURITY_L2);
      if (err) {
        LOG_ERR("Failed to set security (err %d)", err);
        ancs_reset_state();
      }
    }
    return BT_GATT_ITER_STOP;  // Stop discovery, we have everything.
  }

  // Continue searching for more characteristics.
  return BT_GATT_ITER_CONTINUE;
}

static void start_discovery(struct bt_conn *conn) {
  ancs.state = ANCS_STATE_DISCOVERING;
  ancs.discover_params.uuid = &ancs_service_uuid.uuid;
  ancs.discover_params.func = discover_func;
  ancs.discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
  ancs.discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
  ancs.discover_params.type = BT_GATT_DISCOVER_PRIMARY;

  if (bt_gatt_discover(conn, &ancs.discover_params) != 0) {
    LOG_ERR("Discovery failed");
    ancs_reset_state();
  }
}

static void subsciption_cb(struct bt_conn *conn, uint8_t err,
                           struct bt_gatt_subscribe_params *params) {
  if (ancs.state == ANCS_STATE_SUBSCRIBING_NS ||
      ancs.state == ANCS_STATE_SUBSCRIBING_DS) {
    if (err) {
      LOG_ERR("%s subscription failed (err %d)",
              params->value_handle == ancs.ns_handle ? "NS" : "DS", err);
      ancs_reset_state();
    } else {
      LOG_INF("%s subscription successful",
              params->value_handle == ancs.ns_handle ? "NS" : "DS");
      if (params->value_handle == ancs.ds_handle) {
        subscribe_to_ns(conn);
      } else {
        ancs.state = ANCS_STATE_ENABLED;
        if (k_msgq_num_used_get(&notification_q) > 0) {
          k_work_submit_to_queue(&ancs_work_q, &req_notif_info_work);
        }
      }
    }
  }
}

static int subscribe_to_ns(struct bt_conn *conn) {
  ancs.state = ANCS_STATE_SUBSCRIBING_NS;
  ancs.ns_sub_params.subscribe = subsciption_cb;
  ancs.ns_sub_params.notify = notif_source_notify_cb;
  ancs.ns_sub_params.value = BT_GATT_CCC_NOTIFY;
  ancs.ns_sub_params.value_handle = ancs.ns_handle;
  ancs.ns_sub_params.ccc_handle = 0;  // Auto-discover CCC
#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
  ancs.ns_sub_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
  ancs.ns_sub_params.disc_params = &ancs.discover_params;
#endif
  atomic_set_bit(ancs.ns_sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

  int err = bt_gatt_subscribe(conn, &ancs.ns_sub_params);
  if (err) {
    subsciption_cb(conn, err, &ancs.ns_sub_params);  // Handle error
  }

  return err;
}

static int subscribe_to_ds(struct bt_conn *conn) {
  ancs.state = ANCS_STATE_SUBSCRIBING_DS;
  ancs.ds_sub_params.subscribe = subsciption_cb;
  ancs.ds_sub_params.notify = data_source_notify_cb;
  ancs.ds_sub_params.value = BT_GATT_CCC_NOTIFY;
  ancs.ds_sub_params.value_handle = ancs.ds_handle;
  ancs.ds_sub_params.ccc_handle = 0;  // Auto-discover CCC
#if defined(CONFIG_BT_GATT_AUTO_DISCOVER_CCC)
  ancs.ds_sub_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
  ancs.ds_sub_params.disc_params = &ancs.discover_params;
#endif
  atomic_set_bit(ancs.ds_sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

  int err = bt_gatt_subscribe(conn, &ancs.ds_sub_params);
  if (err) {
    subsciption_cb(conn, err, &ancs.ds_sub_params);  // Handle error
  }

  return err;
}

static void discovery_work_handler(struct k_work *work) {
  ARG_UNUSED(work);
  LOG_INF("Starting ANCS service discovery");
  if (ancs.conn) {
    start_discovery(ancs.conn);
  }
}

/*** Attribute Parsing Logic ***/

static void process_notification_attributes(const uint8_t *data,
                                            uint16_t length) {
  // Append new data to our buffer
  if (ancs.ds_buffer_len + length > DATA_SOURCE_BUFFER_SIZE) {
    LOG_ERR(
        "Data source buffer overflow. Data length: %d, buffer length: %d, "
        "dropping data",
        length, ancs.ds_buffer_len);
    ancs.ds_buffer_len = 0;  // Reset buffer to prevent corruption
    return;
  }
  memcpy(&ancs.ds_buffer[ancs.ds_buffer_len], data, length);
  ancs.ds_buffer_len += length;

  uint8_t *buf = ancs.ds_buffer;
  size_t remaining_len = ancs.ds_buffer_len;

  // Start of a new notification attribute response
  if (ancs.current_notif_idx == -1) {
    if (remaining_len < 5) {
      return;  // Need at least CommandID + UID
    }

    if (buf[0] != COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES) {
      LOG_WRN("Unexpected Command ID: %d", buf[0]);
      goto parse_done;
    }

    uint32_t uid = sys_get_le32(&buf[1]);
    k_mutex_lock(&ancs_pool_mutex, K_FOREVER);
    ancs.current_notif_idx = find_pool_slot_by_uid(uid);
    k_mutex_unlock(&ancs_pool_mutex);

    if (ancs.current_notif_idx < 0) {
      LOG_WRN("Attributes for unknown UID 0x%x received", uid);
      goto parse_done;
    }

    buf += 5;
    remaining_len -= 5;
    ancs.remain_num_attr = ancs.num_attr_requested;
  }

  // Process attributes in a loop (TLV format)
  while (ancs.remain_num_attr > 0 && remaining_len >= 3) {
    uint8_t attr_id = buf[0];
    uint16_t attr_len = sys_get_le16(&buf[1]);

    if (remaining_len < (size_t)3 + attr_len) {
      break;  // Not enough data for this attribute, wait for more
    }

    char *target_str = NULL;
    size_t max_len = 0;
    struct ancs_notification *notif = &ancs.notif_pool[ancs.current_notif_idx];

    switch (attr_id) {
      case ATTR_ID_APP_IDENTIFIER:
        target_str = notif->app_identifier;
        max_len = sizeof(notif->app_identifier) - 1;
        break;
      case ATTR_ID_TITLE:
        target_str = notif->title;
        max_len = sizeof(notif->title) - 1;
        break;
      case ATTR_ID_SUBTITLE:
        target_str = notif->subtitle;
        max_len = sizeof(notif->subtitle) - 1;
        break;
      case ATTR_ID_MESSAGE:
        target_str = notif->message;
        max_len = sizeof(notif->message) - 1;
        break;
      case ATTR_ID_DATE:
        target_str = notif->date;
        max_len = sizeof(notif->date) - 1;
        break;
      case ATTR_ID_POSITIVE_ACTION_LABEL:
        target_str = notif->positive_action_label;
        max_len = sizeof(notif->positive_action_label) - 1;
        break;
      case ATTR_ID_NEGATIVE_ACTION_LABEL:
        target_str = notif->negative_action_label;
        max_len = sizeof(notif->negative_action_label) - 1;
        break;
    }

    if (target_str) {
      size_t copy_len = MIN(attr_len, max_len);
      memcpy(target_str, &buf[3], copy_len);
      target_str[copy_len] = '\0';
    }

    buf += 3 + attr_len;
    remaining_len -= (3 + attr_len);
    ancs.remain_num_attr--;
  }

  if (ancs.remain_num_attr == 0) {
    LOG_DBG(
        "Notification parsed: UID=0x%x, AppID = %s, Title=%s, SubTitle=%s, "
        "Message=%s, "
        "Date=%s, Positive=%s, Negative=%s",
        ancs.notif_pool[ancs.current_notif_idx].source.notification_uid,
        ancs.notif_pool[ancs.current_notif_idx].app_identifier,
        ancs.notif_pool[ancs.current_notif_idx].title,
        ancs.notif_pool[ancs.current_notif_idx].subtitle,
        ancs.notif_pool[ancs.current_notif_idx].message,
        ancs.notif_pool[ancs.current_notif_idx].date,
        ancs.notif_pool[ancs.current_notif_idx].positive_action_label,
        ancs.notif_pool[ancs.current_notif_idx].negative_action_label);

    if (ancs.app_cb && ancs.app_cb->on_new_notification) {
      ancs.app_cb->on_new_notification(
          &ancs.notif_pool[ancs.current_notif_idx]);
    }
    goto parse_done;
  }

  // Shift remaining partial data to the start of the buffer
  if (remaining_len > 0 && remaining_len < ancs.ds_buffer_len) {
    memmove(ancs.ds_buffer, buf, remaining_len);
  }
  ancs.ds_buffer_len = remaining_len;
  return;

parse_done:
  // Reset parser state for the next notification
  k_mutex_lock(&ancs_pool_mutex, K_FOREVER);
  if (ancs.current_notif_idx >= 0) {
    ancs.pool_in_use[ancs.current_notif_idx] = false;
    memset(&ancs.notif_pool[ancs.current_notif_idx], 0,
           sizeof(struct ancs_notification));
  }
  k_mutex_unlock(&ancs_pool_mutex);

  ancs.current_notif_idx = -1;
  ancs.ds_buffer_len = 0;
}

static void bt_ancs_cp_write_callback(struct bt_conn *conn, uint8_t err,
                                      struct bt_gatt_write_params *params) {
  LOG_ERR_OR_DBG(err, "Control Point write %s, %d",
                 err ? "failed" : "successful", err);
  k_sem_give(&ancs_data_sem);
}

/*** Work Handler for Requesting Attributes ***/

static void req_notif_info_work_handler(struct k_work *work) {
  uint32_t uid;
  LOG_DBG("Requesting notification attributes");
  while (k_msgq_get(&notification_q, &uid, K_NO_WAIT) == 0) {
    LOG_DBG("Requesting attributes for UID 0x%x", uid);
    uint8_t request[18];
    request[0] = COMMAND_ID_GET_NOTIFICATION_ATTRIBUTES;
    sys_put_le32(uid, &request[1]);
    request[5] = ATTR_ID_APP_IDENTIFIER;
    request[6] = ATTR_ID_TITLE;
    sys_put_le16(CONFIG_ANCS_TITLE_MAX_LEN, &request[7]);  // Max len for title
    request[9] = ATTR_ID_SUBTITLE;
    sys_put_le16(CONFIG_ANCS_SUBTITLE_MAX_LEN,
                 &request[10]);  // Max len for subtitle
    request[12] = ATTR_ID_MESSAGE;
    sys_put_le16(CONFIG_ANCS_MESSAGE_MAX_LEN,
                 &request[13]);  // Max len for message
    request[15] = ATTR_ID_DATE;
    request[16] = ATTR_ID_POSITIVE_ACTION_LABEL;
    request[17] = ATTR_ID_NEGATIVE_ACTION_LABEL;

    ancs.num_attr_requested = 7;

    static struct bt_gatt_write_params write_params = {0};

    write_params.func = bt_ancs_cp_write_callback;
    write_params.handle = ancs.cp_handle;
    write_params.offset = 0;
    write_params.data = request;
    write_params.length = sizeof(request);

    int err = bt_gatt_write(ancs.conn, &write_params);
    if (err) {
      LOG_ERR("Failed to request attributes for UID 0x%x (err %d)", uid, err);
    } else {
      LOG_DBG("Requested attributes for UID 0x%x", uid);
    }
    // Waiting for the write to complete
    if (k_sem_take(&ancs_data_sem, K_MSEC(1000)) == 0) {
      LOG_DBG("Control point write done. Continue for the next notif");
    } else {
      LOG_ERR("Control point write timeout");
    }
  }
}

/*** Public API and Connection Management ***/

int ancs_perform_action(uint32_t notification_uid, ancs_action_id_t action) {
  if (ancs.state != ANCS_STATE_ENABLED) {
    return -EPERM;
  }

  uint8_t request[6] = {
      [0] = COMMAND_ID_PERFORM_NOTIFICATION_ACTION,
      // UID will be placed at [1]
      [5] = action,
  };
  sys_put_le32(notification_uid, &request[1]);

  static struct bt_gatt_write_params write_params = {0};

  write_params.func = bt_ancs_cp_write_callback;
  write_params.handle = ancs.cp_handle;
  write_params.offset = 0;
  write_params.data = request;
  write_params.length = sizeof(request);

  // Take semaphore to block other writes.
  k_sem_take(&ancs_data_sem, K_MSEC(1000));
  if (ancs.conn == NULL) {
    return -EPERM;
  }

  return bt_gatt_write(ancs.conn, &write_params);
}

static void ancs_reset_state(void) {
  LOG_INF("Resetting ANCS state");
  ancs.conn = NULL;
  ancs.state = ANCS_STATE_IDLE;
  ancs.ns_handle = 0;
  ancs.cp_handle = 0;
  ancs.ds_handle = 0;
  ancs.ds_buffer_len = 0;
  ancs.current_notif_idx = -1;

  k_mutex_lock(&ancs_pool_mutex, K_FOREVER);
  for (int i = 0; i < NOTIFICATION_POOL_SIZE; i++) {
    ancs.pool_in_use[i] = false;
  }
  k_mutex_unlock(&ancs_pool_mutex);

  k_msgq_purge(&notification_q);
  // Clear the ancs_data_sem if any
  if (k_sem_count_get(&ancs_data_sem) == 1) {
    k_sem_take(&ancs_data_sem, K_NO_WAIT);
  }
}

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
};
static const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void mtu_updated(struct bt_conn *conn, uint16_t tx, uint16_t rx) {
  LOG_DBG("Updated MTU: TX: %d RX: %d bytes", tx, rx);
}

static struct bt_gatt_cb gatt_callbacks = {
    .att_mtu_updated = mtu_updated,
};

int ancs_client_init(void) {
  if (IS_ENABLED(CONFIG_SETTINGS)) {
    settings_load();
  }

  int err = bt_enable(NULL);
  if (err) {
    LOG_ERR("Bluetooth init failed (err %d)", err);
    return err;
  }

  bt_gatt_cb_register(&gatt_callbacks);

  err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd,
                        ARRAY_SIZE(sd));
  if (err) {
    printk("Advertising failed to start (err %d)\n", err);
    return 0U;
  }

  printk("Advertising successfully started\n");

  LOG_INF("Initializing ANCS Client");
  if (ancs.is_init == false) {
    ancs.current_notif_idx = -1;  // Initialize parser state
    k_work_init(&req_notif_info_work, req_notif_info_work_handler);
    k_work_init_delayable(&discovery_work, discovery_work_handler);

    k_work_queue_init(&ancs_work_q);
    k_work_queue_start(&ancs_work_q, ancs_stack,
                       K_KERNEL_STACK_SIZEOF(ancs_stack),
                       K_PRIO_COOP(CONFIG_ANCS_WORK_QUEUE_PRIORITY), NULL);
    k_thread_name_set(&ancs_work_q.thread, "ancs_work_q");
    ancs.is_init = true;
  }
  LOG_INF("ANCS Client initialized");

  return 0;
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
                             enum bt_security_err err) {
  if (err) {
    LOG_WRN("Security failed: %d", err);
    struct bt_conn_info info = {0};
    bt_conn_get_info(conn, &info);
    bt_unpair(info.id, info.le.dst);

    return;
  }
  LOG_INF("Security changed: level %d", level);

  if (level >= BT_SECURITY_L2 && ancs.state == ANCS_STATE_START_SUBSCRIPTIONS) {
    // Subscribe to Data Source now as security is sufficient
    subscribe_to_ds(conn);
  }
}

static void connected(struct bt_conn *conn, uint8_t err) {
  if (err) {
    return;
  }
  LOG_INF("Connected");
  ancs_reset_state();
  ancs.conn = conn;

  /* Workaround for the error of Android. If we call discovery immediately after
   * connection, it will halt by no response for updating connection param */
  k_work_schedule_for_queue(&ancs_work_q, &discovery_work, K_SECONDS(5));
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
  LOG_INF("Disconnected (reason 0x%02x)", reason);
  ancs_reset_state();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .security_changed = security_changed,
    .connected = connected,
    .disconnected = disconnected,
};

int ancs_register_cb(const struct ancs_callbacks *cb) {
  if (!cb) {
    return -EINVAL;
  }
  ancs.app_cb = cb;

  return 0;
}

// SYS_INIT(ancs_client_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
