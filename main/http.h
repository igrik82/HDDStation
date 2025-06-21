#pragma once

#include "cJSON.h" // IWYU pragma: keep
#include "esp_err.h" // IWYU pragma: keep
#include "esp_event.h" // IWYU pragma: keep
#include "esp_http_server.h"
#include "esp_log.h" // IWYU pragma: keep
#include "esp_spiffs.h" // IWYU pragma: keep
#include "freertos/task.h"
#include "secrets.h" // IWYU pragma: keep
#include <cstddef> // IWYU pragma: keep
#include <esp_http_server.h>
#include <sys/param.h>
#include <sys/stat.h>

namespace Http_NS {
class HttpServer {
private:
    static void _connect_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);
    static void _disconnect_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);

public:
    HttpServer(CalibrationData_t* calibration_data);
    ~HttpServer(void);
    static httpd_handle_t _server;
    constexpr static const char* TAG = "HTTPServer";
    constexpr static const char* TAG_SPIFF = "SPIFFS";
    static esp_vfs_spiffs_conf_t spiffs_config;

    static CalibrationData_t* _calibration_data;

    static esp_err_t start_webserver(void);
    static void stop_webserver(void);
};
} // Namespace Http_NS
