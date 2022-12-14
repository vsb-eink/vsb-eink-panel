#include "esp_log.h"
#include "esp_mac.h"

#include "config.hpp"
#include "utils.hpp"

static constexpr auto* TAG = "config-store";

Config::Config():
    initialized(false),
    nvsHandle(nullptr) {}

esp_err_t Config::set_defaults() {
    esp_err_t err;
    size_t item_size;

    err = this->nvsHandle->get_item_size(nvs::ItemType::SZ, "wifi_ssid", item_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Setting default wifi_ssid");
        err = this->nvsHandle->set_string("wifi_ssid", CONFIG_ESP_WIFI_SSID_FALLBACK);
    }
    if (err != ESP_OK) {
        return err;
    }

    err = this->nvsHandle->get_item_size(nvs::ItemType::SZ, "wifi_password", item_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Setting default wifi_password");
        err = this->nvsHandle->set_string("wifi_password", CONFIG_ESP_WIFI_PASSWORD_FALLBACK);
    }
    if (err != ESP_OK) {
        return err;
    }

    err = this->nvsHandle->get_item_size(nvs::ItemType::SZ, "websocket_url", item_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Setting default websocket_url");
        err = this->nvsHandle->set_string("websocket_url", CONFIG_VSB_EINK_WEBSOCKET_URL_FALLBACK);
    }
    if (err != ESP_OK) {
        return err;
    }

    return this->nvsHandle->commit();
}

esp_err_t Config::init() {
    esp_err_t err;
    this->nvsHandle = nvs::open_nvs_handle("vsb_eink", NVS_READWRITE, &err);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    err = this->set_defaults();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set defaults: %s", esp_err_to_name(err));
        return err;
    }

    this->initialized = true;

    return ESP_OK;
}

void Config::checkInitialized() const {
    if (!this->initialized) {
        throw std::runtime_error("ConfigStore not initialized");
    }
}

void Config::commit() {
    this->checkInitialized();

    esp_err_t err = this->nvsHandle->commit();

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS handle: %s", esp_err_to_name(err));
        throw std::runtime_error("Failed to commit NVS handle");
    }
}

std::string Config::get_wifi_ssid() const {
    this->checkInitialized();

    esp_err_t err;
    size_t ssid_len = 0;

    err = this->nvsHandle->get_item_size(nvs::ItemType::SZ, "wifi_ssid", ssid_len);
    if (err != ESP_OK) {
        throw std::runtime_error(string_format("Failed to get item size: %s", esp_err_to_name(err)));
    }

    auto ssid_buffer = std::make_unique<char[]>(ssid_len);
    err = this->nvsHandle->get_string("wifi_ssid", ssid_buffer.get(), ssid_len);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get wifi ssid: %s", esp_err_to_name(err));
        throw std::runtime_error("Failed to get wifi ssid");
    }

    return { ssid_buffer.get() };
}

esp_err_t Config::set_wifi_ssid(const std::string_view& ssid) {
    this->checkInitialized();

    esp_err_t err;

    err = this->nvsHandle->set_string("wifi_ssid", ssid.data());

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set wifi ssid: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

std::string Config::get_wifi_password() const {
    this->checkInitialized();

    esp_err_t err;
    size_t password_len = 0;

    err = this->nvsHandle->get_item_size(nvs::ItemType::SZ, "wifi_password", password_len);
    if (err != ESP_OK) {
        throw std::runtime_error(string_format("Failed to get item size: %s", esp_err_to_name(err)));
    }

    auto password_buffer = std::make_unique<char[]>(password_len);
    err = this->nvsHandle->get_string("wifi_password", password_buffer.get(), password_len);

    if (err != ESP_OK) {
        throw std::runtime_error(string_format("Failed to get wifi password: %s", esp_err_to_name(err)));
    }

    return { password_buffer.get() };
}

esp_err_t Config::set_wifi_password(const std::string_view& password) {
    this->checkInitialized();

    esp_err_t err;

    err = this->nvsHandle->set_string("wifi_password", password.data());

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set wifi password: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

std::string Config::get_websocket_url() const {
    this->checkInitialized();

    esp_err_t err;
    size_t url_len = 0;

    err = this->nvsHandle->get_item_size(nvs::ItemType::SZ, "websocket_url", url_len);
    if (err != ESP_OK) {
        throw std::runtime_error(string_format("Failed to get item size: %s", esp_err_to_name(err)));
    }

    auto url_buffer = std::make_unique<char[]>(url_len);
    err = this->nvsHandle->get_string("websocket_url", url_buffer.get(), url_len);

    if (err != ESP_OK) {
        throw std::runtime_error(string_format("Failed to get websocket url: %s", esp_err_to_name(err)));
    }

    return { url_buffer.get() };
}

esp_err_t Config::set_websocket_url(const std::string_view& url) {
    if (!this->initialized) {
        ESP_LOGE(TAG, "ConfigStore not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = this->nvsHandle->set_string("websocket_url", url.data());

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set websocket url: %s", esp_err_to_name(err));
        return err;
    }

    return ESP_OK;
}

std::string Config::get_panel_id() const {
    auto buffer = new uint8_t[16];
    esp_base_mac_addr_get(buffer);

    auto mac = string_format("%02x:%02x:%02x:%02x:%02x:%02x", buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
    delete[] buffer;

    return mac;
}
