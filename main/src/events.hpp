#pragma once

ESP_EVENT_DECLARE_BASE(EINK_INTERNAL_EVENT_BASE);

enum class EInkPanelEvent {
    // 0-7 are reserved for basic device messages
    NOOP = 0,
    REBOOT = 1,
    // 8-31 are reserved for system messages
    PERFORM_OTA_UPDATE = 8,
    CONFIG_SET_SSID = 9,
    CONFIG_SET_PASSWORD = 10,
    CONFIG_SET_WEBSOCKET_URL = 11,
    // 32-63 are reserved for bitmap messages
    DRAW_BITMAP_1BIT = 32,
    DRAW_BITMAP_3BIT = 33,
};

typedef struct {
    char *data_ptr;                                  /*!< Data pointer */
    unsigned int data_len;                           /*!< Data length */
    unsigned int payload_len;                        /*!< Total payload length, payloads exceeding buffer will be posted through multiple events */
    unsigned int payload_offset;                     /*!< Actual offset for the data associated with this event */
} eink_panel_event_data_t;
