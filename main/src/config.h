#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <set>

#include <esp_err.h>
#include <nvs.h>
#include <nvs_handle.hpp>

struct WifiConfig {
    std::string ssid;
    std::string password;
};

struct PanelConfig {
    std::string panel_id;
    uint8_t waveform;
};

struct MqttConfig {
    std::string broker_url;
};

class Config {
private:
    std::optional<std::string> get_string(const std::shared_ptr<nvs::NVSHandle> &nvs_handle, const char* item_key);
    static std::string get_default_panel_id();
public:
    Config();
    esp_err_t load_from_nvs();
    esp_err_t commit();

    void set_wifi_config(const WifiConfig &config);
    void set_panel_config(const PanelConfig &config);
    void set_mqtt_config(const MqttConfig &config);

    esp_err_t commit_wifi_config();
    esp_err_t commit_panel_config();
    esp_err_t commit_mqtt_config();

    void rollback_wifi_config();
    void rollback_panel_config();
    void rollback_mqtt_config();

    WifiConfig wifi;
    WifiConfig wifi_fallback;

    PanelConfig panel;

    MqttConfig mqtt;
    MqttConfig mqtt_fallback;
};