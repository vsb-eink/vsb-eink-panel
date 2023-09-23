#pragma once

#include <esp_err.h>
#include <nvs.h>
#include <nvs_handle.hpp>
#include <memory>

class Config {
private:
    bool initialized;
    std::unique_ptr<nvs::NVSHandle> nvsHandle;

    esp_err_t set_defaults();
    void checkInitialized() const;

    static std::string get_default_panel_id() ;
public:
    Config();
    esp_err_t init();

    std::string get_wifi_ssid() const;
    esp_err_t set_wifi_ssid(const std::string_view& ssid);

    std::string get_wifi_password() const;
    esp_err_t set_wifi_password(const std::string_view& password);

    std::string get_websocket_url() const;
    esp_err_t set_websocket_url(const std::string_view& url);

    std::string get_panel_id() const;
    esp_err_t set_panel_id(const std::string_view& panel_id);

    void commit();
};