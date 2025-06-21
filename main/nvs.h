#pragma once

#include "esp_err.h"
#include "esp_log.h" // IWYU pragma: keep
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h" // IWYU pragma: keep
#include <cstdint>
#include <string>

namespace Nvs_NS {
class Nvs {
protected:
    nvs_handle _nvs_handle;
    SemaphoreHandle_t _mutex;

public:
    Nvs(const std::string storage_name);
    ~Nvs(void);

    esp_err_t read_str(const std::string key, char* value,
        std::string default_value);
    esp_err_t write_str(const std::string key, char* value);
    esp_err_t read_u32(const std::string key, uint32_t& value,
        uint32_t& default_value);
    esp_err_t write_u32(const std::string key, uint32_t& value);
    esp_err_t read_float(const std::string key, float& value,
        float& default_value);
    esp_err_t write_float(const std::string key, float& value);

    constexpr static const char* TAG = "NVS";
};
} // namespace Nvs_NS
