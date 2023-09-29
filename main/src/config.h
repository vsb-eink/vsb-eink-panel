#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <set>

#include <esp_err.h>
#include <nvs.h>
#include <nvs_handle.hpp>

struct NetworkConfig {
    std::string wifi_ssid;
    std::string wifi_password;

    static constexpr const char* NVS_WIFI_SSID = "wifi_ssid";
    static constexpr const char* NVS_WIFI_PASSWORD = "wifi_password";
};

struct PanelConfig {
    std::string panel_id;

    static constexpr const char* NVS_PANEL_ID = "panel_id";
};

struct MqttConfig {
    std::string broker_url;

    static constexpr const char* NVS_BROKER_URL = "mqtt_broker_url";
};

class Config {
private:
    std::optional<std::string> get_string(const std::shared_ptr<nvs::NVSHandle> &nvs_handle, const char* item_key);
    static std::string get_default_panel_id();
public:
    Config();
    esp_err_t load_from_nvs();
    esp_err_t commit();

    void set_network_config(const NetworkConfig &config);
    void set_panel_config(const PanelConfig &config);
    void set_mqtt_config(const MqttConfig &config);

    esp_err_t commit_network_config();
    esp_err_t commit_panel_config();
    esp_err_t commit_mqtt_config();

    void rollback_network_config();
    void rollback_panel_config();
    void rollback_mqtt_config();

    NetworkConfig network;
    NetworkConfig network_fallback;

    PanelConfig panel;
    PanelConfig panel_fallback;

    MqttConfig mqtt;
    MqttConfig mqtt_fallback;
};