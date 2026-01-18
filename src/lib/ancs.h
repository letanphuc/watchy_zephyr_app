#ifndef ANCS_H_
#define ANCS_H_

#include <zephyr/types.h>
#include <stddef.h>

/**
 * @file ancs.h
 * @brief ANCS Client (Consumer) for Zephyr.
 */

/* ANCS event and category IDs, matching the Apple specification. */


#define CONFIG_ANCS_APP_ID_MAX_LEN          (48)
#define CONFIG_ANCS_TITLE_MAX_LEN           (100)
#define CONFIG_ANCS_SUBTITLE_MAX_LEN        (100)
#define CONFIG_ANCS_MESSAGE_MAX_LEN         (256)
#define CONFIG_ANCS_DATE_MAX_LEN            (32)
#define CONFIG_ANCS_ACTION_LABEL_MAX_LEN    (16)

typedef enum {
    ANCS_EVENT_ID_NOTIFICATION_ADDED = 0,
    ANCS_EVENT_ID_NOTIFICATION_MODIFIED = 1,
    ANCS_EVENT_ID_NOTIFICATION_REMOVED = 2,
} ancs_event_id_t;

typedef enum {
    ANCS_EVENT_FLAG_SILENT = (1 << 0),
    ANCS_EVENT_FLAG_IMPORTANT = (1 << 1),
    ANCS_EVENT_FLAG_PRE_EXISTING = (1 << 2),
    ANCS_EVENT_FLAG_POSITIVE_ACTION = (1 << 3),
    ANCS_EVENT_FLAG_NEGATIVE_ACTION = (1 << 4),
} ancs_event_flags_t;

typedef enum {
    ANCS_CATEGORY_ID_OTHER = 0,
    ANCS_CATEGORY_ID_INCOMING_CALL = 1,
    ANCS_CATEGORY_ID_MISSED_CALL = 2,
    ANCS_CATEGORY_ID_VOICEMAIL = 3,
    ANCS_CATEGORY_ID_SOCIAL = 4,
    ANCS_CATEGORY_ID_SCHEDULE = 5,
    ANCS_CATEGORY_ID_EMAIL = 6,
    ANCS_CATEGORY_ID_NEWS = 7,
    ANCS_CATEGORY_ID_HEALTH_AND_FITNESS = 8,
    ANCS_CATEGORY_ID_BUSINESS_AND_FINANCE = 9,
    ANCS_CATEGORY_ID_LOCATION = 10,
    ANCS_CATEGORY_ID_ENTERTAINMENT = 11,
    ANCS_CATEGORY_ID_ACTIVE_CALL = 12,
} ancs_category_id_t;

typedef enum {
    ANCS_ACTION_ID_POSITIVE = 0,
    ANCS_ACTION_ID_NEGATIVE = 1,
} ancs_action_id_t;

/**
 * @brief Structure for a parsed ANCS notification source event.
 */
struct ancs_notification_source {
    ancs_event_id_t event_id;
    uint8_t event_flags;
    ancs_category_id_t category_id;
    uint8_t category_count;
    uint32_t notification_uid;
};

/**
 * @brief Structure holding the full details of an iOS notification.
 *
 * @note Kconfig options should be used to define max lengths for strings
 * to control memory usage, e.g., CONFIG_ANCS_TITLE_MAX_LEN.
 */
struct ancs_notification {
    struct ancs_notification_source source;
    char app_identifier[CONFIG_ANCS_APP_ID_MAX_LEN + 1];
    char title[CONFIG_ANCS_TITLE_MAX_LEN + 1];
    char subtitle[CONFIG_ANCS_SUBTITLE_MAX_LEN + 1];
    char message[CONFIG_ANCS_MESSAGE_MAX_LEN + 1];
    char date[CONFIG_ANCS_DATE_MAX_LEN + 1];
    char positive_action_label[CONFIG_ANCS_ACTION_LABEL_MAX_LEN + 1];
    char negative_action_label[CONFIG_ANCS_ACTION_LABEL_MAX_LEN + 1];
};

/**
 * @brief ANCS client callback structure.
 *
 * Register an instance of this structure to receive ANCS events.
 */
struct ancs_callbacks {
    /**
     * @brief Called when a new or updated notification with all its attributes is ready.
     * @param notif Pointer to the notification structure. This pointer is valid only
     * within the scope of this callback.
     */
    void (*on_new_notification)(const struct ancs_notification *notif);

    /**
     * @brief Called when a notification is removed from the iOS device.
     * @param notification_uid The UID of the removed notification.
     */
    void (*on_notification_removed)(uint32_t notification_uid);
};

/**
 * @brief Initialize the ANCS client module.
 *
 * This function sets up the necessary Bluetooth connection callbacks 
 * and prepares the ANCS client for operation.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ancs_client_init(void);

/**
 * @brief Register the callback for the ANCS client module.
 *
 * This function sets up the necessary Bluetooth connection callbacks 
 *
 * @param cb Pointer to a struct of application-defined callbacks.
 * @return 0 on success, or a negative error code on failure.
 */
int ancs_register_cb(const struct ancs_callbacks *cb);

/**
 * @brief Perform an action on a notification.
 *
 * @param notification_uid The UID of the notification to act upon.
 * @param action The action to perform (Positive or Negative).
 * @return 0 on success, or a negative error code on failure.
 */
int ancs_perform_action(uint32_t notification_uid, ancs_action_id_t action);

#endif /* ANCS_H_ */