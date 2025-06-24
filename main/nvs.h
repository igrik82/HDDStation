#pragma once

#include "esp_err.h"
#include "esp_log.h" // IWYU pragma: keep
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h" // IWYU pragma: keep
#include <cstdint>

namespace Nvs_NS {

typedef union {
    float float_num;
    uint8_t bytes_dump[sizeof(float)];
} float_dump_t;

class Nvs {
protected:
    nvs_handle _nvs_handle;
    SemaphoreHandle_t _mutex;

public:
    Nvs(const char* storage_name);
    ~Nvs(void);

    esp_err_t read_str(const char* key, char* value, const char* default_value);
    esp_err_t write_str(const char* key, const char* value);
    esp_err_t read_u32(const char* key, uint32_t* value, uint32_t* default_value);
    esp_err_t write_u32(const char* key, uint32_t* value);
    esp_err_t read_float(const char* key, float* value, float* default_value);
    esp_err_t write_float(const char* key, float* value);

    constexpr static const char* TAG = "NVS";
};
} // namespace Nvs_NS
