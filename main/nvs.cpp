#include "nvs.h"
#include "esp_err.h"

namespace Nvs_NS {

Nvs::Nvs(const std::string storage_name)
{
    _mutex = xSemaphoreCreateMutex();
    esp_err_t err = nvs_open(storage_name.c_str(), NVS_READWRITE, &_nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
    }
    ESP_LOGI(TAG, "NVS opened successfully");
};

Nvs::~Nvs(void)
{
    nvs_close(_nvs_handle);
    vSemaphoreDelete(_mutex);
};

// Read string from NVS
esp_err_t Nvs::read_str(const std::string key, char* value,
    std::string default_value)
{
    // Take mutex
    if (xSemaphoreTake(_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_FAIL;
    }

    // Get required size
    size_t required_size {};
    esp_err_t err = nvs_get_str(_nvs_handle, key.c_str(), nullptr, &required_size);

    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Size query failed for key '%s': %s", key.c_str(),
            esp_err_to_name(err));
        xSemaphoreGive(_mutex);
        return err;
    }

    // If key not found
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Key '%s' not found. Writing default value.", key.c_str());

        esp_err_t err = write_str(key, &default_value[0]);

        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Write failed for key '%s': %s", key.c_str(),
                esp_err_to_name(err));

            xSemaphoreGive(_mutex);
            return err;
        }

        xSemaphoreGive(_mutex);
        return err;
    }

    // Because str.data() before C++17 returned "const char*"
    // used  &value[0] construction for referen
    err = nvs_get_str(_nvs_handle, key.c_str(), value, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read failed for key '%s': %s", key.c_str(),
            esp_err_to_name(err));

        xSemaphoreGive(_mutex);
        return err;
    }

    ESP_LOGI(TAG, "Successfully read key '%s' with value '%s'", key.c_str(),
        value);

    xSemaphoreGive(_mutex);
    return err;
};

// Write string to NVS
esp_err_t Nvs::write_str(const std::string key, char* value)
{
    esp_err_t err = nvs_set_str(_nvs_handle, key.c_str(), value);
    if (err == ESP_OK) {
        nvs_commit(_nvs_handle);
    }
    return err;
};

} // namespace Nvs_NS
