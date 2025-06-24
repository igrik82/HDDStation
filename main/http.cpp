#include "http.h"
#include "nvs.h"
#include "secrets.h"
#include <cstdint>

namespace Http_NS {

httpd_handle_t HttpServer::_server = NULL;
esp_vfs_spiffs_conf_t HttpServer::spiffs_config;

HttpServer::HttpServer(void)
{

    // initialize and mounting SPIFFS
    spiffs_config.base_path = "/spiffs";
    spiffs_config.partition_label = "storage";
    spiffs_config.max_files = 5;
    spiffs_config.format_if_mount_failed = true;
    esp_err_t ret = esp_vfs_spiffs_register(&spiffs_config);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_SPIFF, "Failed to mount or format filesystem");
        } else {
            ESP_LOGE(TAG_SPIFF, "Failed to initialize SPIFFS (%s)",
                esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI("SPIFFS", "Finished mounting SPIFFS");

    // get SPIFFS partition information
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(spiffs_config.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SPIFF, "Failed to get SPIFFS partition information (%s)",
            esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG_SPIFF, "Partition size: total: %zu, used: %zu", total, used);
    }

    // // Register event handler for starting http server
    // ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
    //     &_connect_handler, this));
    // ESP_ERROR_CHECK(esp_event_handler_register(
    //     WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &_disconnect_handler, this));
}

HttpServer::~HttpServer(void)
{
    if (_server != NULL) {
        stop_webserver();
        ESP_LOGI(TAG, "Server is destructed");
    }
}

// ======================== Root address handler "/" ========================
/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t* req)
{
    char html_file[] = "/spiffs/index.html";

    // Check if destination file exists before renaming
    ESP_LOGI(HttpServer::TAG_SPIFF, "Trying to open %s", html_file);
    FILE* fd = fopen(html_file, "r");
    if (!fd) {
        ESP_LOGE(HttpServer::TAG_SPIFF, "Failed to open %s", html_file);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    ESP_LOGI("SPIFFS", "File %s opened successfully", html_file);

    char buffer[512];
    size_t bytes_read;
    struct stat file_stat;

    if (stat(html_file, &file_stat) != 0) {
        ESP_LOGE(HttpServer::TAG, "Failed to get file stats");
        fclose(fd);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_LOGI(HttpServer::TAG, "Sending file: %s (%ld bytes)...", html_file,
        file_stat.st_size);

    // // Set the content type header
    // httpd_resp_set_type(req, "image/x-icon");

    // Send the file
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fd)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, bytes_read) != ESP_OK) {
            ESP_LOGE(HttpServer::TAG, "File sending failed!");
            fclose(fd);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    }
    fclose(fd);

    // Always finish sending the response
    httpd_resp_send_chunk(req, NULL, 0);

    ESP_LOGI(HttpServer::TAG, "File sending complete");
    return ESP_OK;
}

httpd_uri_t root_dir = { .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL };

// ======================== "/get-wifi-settings" ========================
static esp_err_t settings_get_handler(httpd_req_t* req)
{

    uint32_t min_hdd_temp = MIN_HDD_TEMP;
    uint32_t max_hdd_temp = MAX_HDD_TEMP;
    uint32_t frequency = FREQUENCY;
    float sensor_0_corr = SENSOR_0;
    float sensor_1_corr = SENSOR_1;
    char wifi_ssid[18] {};
    char wifi_password[24] {};
    char mqtt_user[18] {};
    char mqtt_password[24] {};
    char ip[16] {};
    uint32_t port = MQTT_PORT;

    Nvs_NS::Nvs nvs(STORAGE_SPACE);
    nvs.read_u32(MIN_HDD_TEMP_KEY, &min_hdd_temp, &min_hdd_temp);
    nvs.read_u32(MAX_HDD_TEMP_KEY, &max_hdd_temp, &max_hdd_temp);
    nvs.read_u32(FREQUENCY_KEY, &frequency, &frequency);
    nvs.read_float(SENSOR_0_KEY, &sensor_0_corr, &sensor_0_corr);
    nvs.read_float(SENSOR_1_KEY, &sensor_1_corr, &sensor_1_corr);
    nvs.read_str(WIFI_SSID_KEY, wifi_ssid, WIFI_SSID);
    nvs.read_str(WIFI_PASSWORD_KEY, wifi_password, WIFI_PASSWORD);
    nvs.read_str(MQTT_HOST_KEY, ip, MQTT_HOST);
    nvs.read_u32(MQTT_PORT_KEY, &port, &port);
    nvs.read_str(MQTT_USER_KEY, mqtt_user, MQTT_USER);
    nvs.read_str(MQTT_PASSWORD_KEY, mqtt_password, MQTT_PASSWORD);

    cJSON* root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "min_temp", min_hdd_temp);
    cJSON_AddNumberToObject(root, "max_temp", max_hdd_temp);
    cJSON_AddNumberToObject(root, "fan_freq", frequency);
    cJSON_AddNumberToObject(root, "sens_corr_0", sensor_0_corr);
    cJSON_AddNumberToObject(root, "sens_corr_1", sensor_1_corr);
    cJSON_AddStringToObject(root, "ssid", wifi_ssid);
    cJSON_AddStringToObject(root, "wifi_password", wifi_password);
    cJSON_AddStringToObject(root, "mqtt_address", ip);
    cJSON_AddNumberToObject(root, "mqtt_port", port);
    cJSON_AddStringToObject(root, "mqtt_user", mqtt_user);
    cJSON_AddStringToObject(root, "mqtt_password", mqtt_password);

    const char* json_str = cJSON_Print(root);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_str, strlen(json_str));

    cJSON_Delete(root);
    free((void*)json_str);
    return ESP_OK;
}

httpd_uri_t settings_get = { .uri = "/get-settings",
    .method = HTTP_GET,
    .handler = settings_get_handler,
    .user_ctx = NULL };
// ======================================================================

void HttpServer::_connect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (HttpServer::_server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        start_webserver();
    }
}

esp_err_t HttpServer::start_webserver(void)
{
    if (_server != NULL) {
        return ESP_OK;
    }
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 2;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&_server, &config) == ESP_OK) {

        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(_server, &root_dir);
        httpd_register_uri_handler(_server, &settings_get);
        // httpd_register_uri_handler(server, &echo);
        // httpd_register_uri_handler(server, &ctrl);
        return ESP_OK;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return ESP_FAIL;
}

void HttpServer::_disconnect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    if (HttpServer::_server) {
        ESP_LOGI(TAG, "Stoping webserver");
        stop_webserver();
    }
}

void HttpServer::stop_webserver(void)
{
    if (_server != NULL) {
        esp_err_t ret = httpd_stop(_server);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop server: %s", esp_err_to_name(ret));
        }
        _server = NULL;
        if (esp_spiffs_mounted(spiffs_config.partition_label)) {
            ESP_ERROR_CHECK(esp_vfs_spiffs_unregister(spiffs_config.partition_label));
            ESP_LOGI(TAG, "SPIFFS unmounted");
        }
        ESP_LOGI(TAG, "Server stopped successfully");
    }
}
} // namespace Http_NS
