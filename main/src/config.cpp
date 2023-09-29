#include "config.h"

#include <nvs_handle.hpp>
#include <esp_mac.h>

#include "utils.h"

Config::Config() {};

std::string Config::get_default_panel_id() {
    auto buffer = new uint8_t[16];
    esp_base_mac_addr_get(buffer);

    auto mac = string_format("%02x:%02x:%02x:%02x:%02x:%02x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
    delete[] buffer;

    return mac;
}

std::optional<std::string> Config::get_string(const std::shared_ptr<nvs::NVSHandle> &nvs_handle, const char * item_key) {
    esp_err_t err;
    size_t string_len;

    err = nvs_handle->get_item_size(nvs::ItemType::SZ, item_key, string_len);
    if (err != ESP_OK) {
        return {};
    }

    std::string result;
    err = nvs_handle->get_string(item_key, result.data(), string_len);
    if (err != ESP_OK) {
        return {};
    }

    return result;
}

esp_err_t Config::load_from_nvs() {
    esp_err_t err;
    std::shared_ptr nvs_handle = nvs::open_nvs_handle("vsb_eink", NVS_READONLY, &err);

    if (err != ESP_OK) {
        return err;
    }

    auto wifi_ssid_fallback = get_string(nvs_handle, "wifi_ssid_fallback");
    network_fallback.wifi_ssid = wifi_ssid_fallback.value_or("TUO-IOT");

    auto wifi_password_fallback = get_string(nvs_handle, "wifi_password_fallback");
    network_fallback.wifi_password = wifi_password_fallback.value_or("");

    auto wifi_ssid = get_string(nvs_handle, "wifi_ssid");
    network.wifi_ssid = wifi_ssid.value_or(network_fallback.wifi_ssid);

    auto wifi_password = get_string(nvs_handle, "wifi_password");
    network.wifi_password = wifi_password.value_or(network_fallback.wifi_password);

    auto panel_id_fallback = get_string(nvs_handle, "panel_id_fallback");
    panel_fallback.panel_id = panel_id_fallback.value_or(get_default_panel_id());

    auto panel_id = get_string(nvs_handle, "panel_id");
    panel.panel_id = panel_id.value_or(panel.panel_id);

    auto mqtt_broker_url_fallback = get_string(nvs_handle, "mqtt_broker_url_fallback");
    mqtt_fallback.broker_url = mqtt_broker_url_fallback.value_or("mqtt://vsb-eink.lksv.cz:1883");

    auto mqtt_broker_url = get_string(nvs_handle, "mqtt_broker_url");
    mqtt.broker_url = mqtt_broker_url.value_or(mqtt_fallback.broker_url);
}

esp_err_t Config::commit() {
    esp_err_t err;
    std::shared_ptr nvs_handle = nvs::open_nvs_handle("vsb_eink", NVS_READONLY, &err);

    if (err != ESP_OK) {
        return err;
    }

    nvs_handle->set_string("wifi_ssid", network.wifi_ssid.c_str());
    nvs_handle->set_string("wifi_password", network.wifi_password.c_str());
    nvs_handle->set_string("wifi_ssid_fallback", network_fallback.wifi_ssid.c_str());
    nvs_handle->set_string("wifi_password_fallback", network_fallback.wifi_password.c_str());
    nvs_handle->set_string("panel_id", panel.panel_id.c_str());
    nvs_handle->set_string("panel_id_fallback", panel_fallback.panel_id.c_str());
    nvs_handle->set_string("mqtt_broker_url", mqtt.broker_url.c_str());
    nvs_handle->set_string("mqtt_broker_url_fallback", mqtt_fallback.broker_url.c_str());

    return nvs_handle->commit();
}

void Config::set_network_config(const NetworkConfig &config) {
    network_fallback = network;
    network = config;
}

void Config::set_panel_config(const PanelConfig &config) {
    panel_fallback = panel;
    panel = config;
}

void Config::set_mqtt_config(const MqttConfig &config) {
    mqtt_fallback = mqtt;
    mqtt = config;
}

esp_err_t Config::commit_network_config() {
    esp_err_t err;
    std::shared_ptr nvs_handle = nvs::open_nvs_handle("vsb_eink", NVS_READWRITE, &err);

    if (err != ESP_OK) {
        return err;
    }
}