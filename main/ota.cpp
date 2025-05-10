#include "ota.h"

namespace Ota_NS {

Ota::~Ota(void)
{
    if (config.url) {
        free((void*)config.url);
        config.url = nullptr;
    }
}

Ota::Ota(const OtaParams& params)
    : firmware_url(params.firmware_url)
{
    config = {};
    config.url = firmware_url.c_str();
}

esp_err_t Ota::start_update(void)
{
    ESP_LOGI(TAG, "Starting OTA from URL: %s", config.url);
    esp_err_t err = esp_https_ota(&config);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Firmware upgrade completed!");
        esp_restart();
        ESP_LOGI(TAG, "Restarting...");
    } else {

        ESP_LOGE(TAG, "Firmware upgrade failed: %d!", err);
        return err;
    }
}

}; // namespace Ota_NS
