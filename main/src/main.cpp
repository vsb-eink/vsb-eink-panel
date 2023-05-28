#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "esp_log.h"

#include "services/inkplate_static.h"

static const char *TAG = "Main";

static void log_error_if_nonzero(const char *message, int error_code) {
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_connected_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Connected to MQTT broker");
}

static void mqtt_disconnected_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "Disconnected from MQTT broker");
}

static void mqtt_subscribed_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;

    ESP_LOGI(TAG, "Subscribed to %.*s", event->topic_len, event->topic);
}

static void mqtt_unsubscribed_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;

    ESP_LOGI(TAG, "Unsubscribed from %.*s", event->topic_len, event->topic);
}

static void mqtt_published_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;

    ESP_LOGI(TAG, "Published to %.*s", event->topic_len, event->topic);
}

static void mqtt_error_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;

    ESP_LOGI(TAG, "Error event: %d", event->error_handle->error_type);

    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
        log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
        log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
        log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
        ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
    }
}

static void mqtt_data_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    auto event = static_cast<esp_mqtt_event_handle_t>(event_data);
    esp_mqtt_client_handle_t client = event->client;

    ESP_LOGI(TAG, "Received data from %.*s", event->topic_len, event->topic);

    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    printf("DATA=%.*s\r\n", event->data_len, event->data);
}

[[noreturn]]
void mainTask(void *param) {
    ESP_LOGI(TAG, "Main task has started");
    auto& inkplate = InkplateStatic::getInstance();
    inkplate.begin();

    inkplate.clearDisplay();
    inkplate.display();

    inkplate.joinAP("TurrisLukasu", "pekamalu");

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = "mqtt://192.168.1.164";
    mqtt_cfg.broker.address.port = 1883;
    mqtt_cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;

    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_CONNECTED, mqtt_connected_event_handler, nullptr);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_DISCONNECTED, mqtt_disconnected_event_handler, nullptr);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_SUBSCRIBED, mqtt_subscribed_event_handler, nullptr);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_UNSUBSCRIBED, mqtt_unsubscribed_event_handler, nullptr);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_PUBLISHED, mqtt_published_event_handler, nullptr);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ERROR, mqtt_error_event_handler, nullptr);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_DATA, mqtt_data_event_handler, nullptr);

    esp_mqtt_client_start(mqtt_client);

    int msg_id;
    msg_id = esp_mqtt_client_subscribe(mqtt_client, "/topic/qos0", 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

    msg_id = esp_mqtt_client_subscribe(mqtt_client, "/topic/qos1", 1);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

    while (true) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


#define STACK_SIZE 10000

extern "C" void app_main() {
    TaskHandle_t xHandle = nullptr;

    xTaskCreate(mainTask, "mainTask", STACK_SIZE, (void *) 1, tskIDLE_PRIORITY, &xHandle);
    configASSERT(xHandle);
}
