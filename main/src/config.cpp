#include "config.h"

#include <nvs_handle.hpp>
#include <esp_mac.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include "utils.h"

Config::Config(): wifi{}, wifi_fallback{}, panel{}, mqtt{}, mqtt_fallback{} {};

std::string Config::get_default_panel_id() {
    uint8_t buffer[6];

    ESP_ERROR_CHECK(esp_efuse_mac_get_default(buffer));

    auto mac = string_format("%02x:%02x:%02x:%02x:%02x:%02x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);

    return mac;
}

std::optional<std::string> Config::get_string(const std::shared_ptr<nvs::NVSHandle> &nvs_handle, const char * item_key) {
    esp_err_t err;
    size_t string_len;

    err = ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_handle->get_item_size(nvs::ItemType::SZ, item_key, string_len));
    if (err != ESP_OK) {
        ESP_LOGW("Config", "Failed to get item size for %s, will use default", item_key);
        return std::nullopt;
    }

    char read_buffer[string_len + 1];
    err = ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_handle->get_string(item_key, read_buffer, string_len));
    if (err != ESP_OK) {
        ESP_LOGW("Config", "Failed to get string for %s, will use default", item_key);
        return std::nullopt;
    }

    return std::string(read_buffer);
}

esp_err_t Config::load_from_nvs() {
    esp_err_t err;
    std::shared_ptr nvs_handle = nvs::open_nvs_handle("vsb_eink", NVS_READONLY, &err);

    if (err != ESP_OK) {
        return err;
    }

    // Network config
    auto wifi_ssid_fallback = get_string(nvs_handle, "wifi_ssid_b");
    wifi_fallback.ssid = wifi_ssid_fallback.value_or("TUO-IOT");

    auto wifi_password_fallback = get_string(nvs_handle, "wifi_pass_b");
    wifi_fallback.password = wifi_password_fallback.value_or("");

    auto wifi_ssid = get_string(nvs_handle, "wifi_ssid_a");
    wifi.ssid = wifi_ssid.value_or(wifi_fallback.ssid);

    auto wifi_password = get_string(nvs_handle, "wifi_pass_a");
    wifi.password = wifi_password.value_or(wifi_fallback.password);

    // Panel config
    auto panel_id = get_string(nvs_handle, "panel_id");
    panel.panel_id = panel_id.value_or(get_default_panel_id());

    err = ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_handle->get_item("waveform", panel.waveform));
    if (err != ESP_OK) {
        panel.waveform = panel.waveform;
    }

    // MQTT config
    auto mqtt_broker_url_fallback = get_string(nvs_handle, "broker_url_b");
    mqtt_fallback.broker_url = mqtt_broker_url_fallback.value_or("mqtt://vsb-eink.lksv.cz:1883");

    auto mqtt_broker_url = get_string(nvs_handle, "broker_url_a");
    mqtt.broker_url = mqtt_broker_url.value_or(mqtt_fallback.broker_url);

    return ESP_OK;
}

esp_err_t Config::commit() {
    esp_err_t err;

    err = commit_wifi_config();
    if (err != ESP_OK) return err;

    err = commit_panel_config();
    if (err != ESP_OK) return err;

    err = commit_mqtt_config();
    if (err != ESP_OK) return err;

    return ESP_OK;
}

void Config::set_wifi_config(const WifiConfig &config) {
    wifi_fallback = wifi;
    wifi = config;
}

void Config::set_panel_config(const PanelConfig &config) {
    panel = config;
}

void Config::set_mqtt_config(const MqttConfig &config) {
    mqtt_fallback = mqtt;
    mqtt = config;
}

void Config::rollback_wifi_config() {
    std::swap(wifi, wifi_fallback);
    commit_wifi_config();
}

void Config::rollback_panel_config() {
    throw std::runtime_error("Not implemented");
}

void Config::rollback_mqtt_config() {
    std::swap(mqtt, mqtt_fallback);
    commit_mqtt_config();
}

esp_err_t Config::commit_wifi_config() {
    esp_err_t err;
    std::shared_ptr nvs_handle = nvs::open_nvs_handle("vsb_eink", NVS_READWRITE, &err);

    if (err != ESP_OK) {
        return err;
    }

    err = nvs_handle->set_string("wifi_ssid_a", wifi.ssid.c_str());
    if (err != ESP_OK) return err;
    err = nvs_handle->set_string("wifi_pass_a", wifi.password.c_str());
    if (err != ESP_OK) return err;
    err = nvs_handle->set_string("wifi_ssid_b", wifi_fallback.ssid.c_str());
    if (err != ESP_OK) return err;
    err = nvs_handle->set_string("wifi_pass_b", wifi_fallback.password.c_str());
    if (err != ESP_OK) return err;

    return nvs_handle->commit();
}

esp_err_t Config::commit_panel_config() {
    esp_err_t err;
    std::shared_ptr nvs_handle = nvs::open_nvs_handle("vsb_eink", NVS_READWRITE, &err);

    if (err != ESP_OK) {
        return err;
    }

    err = nvs_handle->set_string("panel_id", panel.panel_id.c_str());
    if (err != ESP_OK) return err;
    err = nvs_handle->set_item("waveform", panel.waveform);
    if (err != ESP_OK) return err;

    return nvs_handle->commit();
}

esp_err_t Config::commit_mqtt_config() {
    esp_err_t err;
    std::shared_ptr nvs_handle = nvs::open_nvs_handle("vsb_eink", NVS_READWRITE, &err);

    if (err != ESP_OK) {
        return err;
    }

    err = nvs_handle->set_string("broker_url_a", mqtt.broker_url.c_str());
    if (err != ESP_OK) return err;
    err = nvs_handle->set_string("broker_url_b", mqtt_fallback.broker_url.c_str());
    if (err != ESP_OK) return err;

    return nvs_handle->commit();
}