#include "FreeRTOS.h" // IWYU pragma: keep
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "fan.h"
#include "freertos/queue.h"
#include "gpio.h" // IWYU pragma: keep
#include "http.h" // IWYU pragma: keep
#include "mqtt.h"
#include "nvs_flash.h"
#include "ota.h"
#include "wifi_simple.h"
#include <cmath>
#include <cstdint>
#include <cstdio>

constexpr uint8_t SENSOR_COUNT { 2 };
uint16_t STACK_TASK_SIZE { 4096 }; // 1024 * 4

// ============================ Global Variables ==============================

// Define function for manage mqtt commands topic
void http_server(void* pvParameter);
void get_temperature(void* pvParameter);
void ota_update(void* pvParameter);

// To avoid 1-Wire interference from the HTTP server on ESP8266, stop
// temperature measurements and run the fan at full power.
volatile bool is_http_running { false };

// TODO: Make class Event Manager
EventGroupHandle_t common_event_group = xEventGroupCreate();

// Queue for fan control must contain at least 6 elements. Because average
// data contain 6 measurements from sensors
QueueHandle_t temperature_queue_PWM = xQueueCreate(10, sizeof(SensorData_t));
// Queue for mqtt - % duty cycle
QueueHandle_t duty_percent_queue = xQueueCreate(5, sizeof(uint8_t));
// Queue for mqtt - temperature
QueueHandle_t temperature_queue = xQueueCreate(5, sizeof(SensorData_t));
// ===================== FreeRTOS Tasks =======================================
// Fan control
TaskHandle_t fan_control_handle = NULL;
void fan_control(void* pvParameter)
{
    // Pin for fan
    uint8_t pin = 13;

    Fan_NS::FanPWM fan(pin, &temperature_queue_PWM, &duty_percent_queue);
    bool set_full_power = { false };
    for (;;) {
        // If http server is running
        if (is_http_running == true) {
            // Turn on the fan
            if (set_full_power == false) {
                set_full_power = true;
                uint8_t power_percent = 100;

                fan.set_duty(fan.get_max_duty());
                ESP_LOGI("Fan", "Fan is turned on for full power");
                if (xQueueSend(duty_percent_queue, &power_percent, 0) != pdPASS) {
                    ESP_LOGE("FAN", "Failed to send duty percent.");
                }
            }
        } else {
            // Wait for at least 6 measurements
            if (uxQueueMessagesWaiting(temperature_queue_PWM) >= 6) {
                fan.start();
            }
            set_full_power = false;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Http server
TaskHandle_t http_server_handle = NULL;
void http_server(void* pvParameter)
{
    Http_NS::HttpServer server;

    if (server.start_webserver() != ESP_OK) {
        is_http_running = false;
        vTaskDelete(NULL);
        return;
    }

    while (is_http_running == true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    server.stop_webserver();
    vTaskDelete(NULL);
}

// Wifi connection
TaskHandle_t wifi_connection_handle = NULL;
void wifi_connection(void* pvParameter)
{
    // Wifi initialization
    Wifi_NS::Wifi wifi;
    for (;;) {
        wifi.start();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Processes three sensor values by discarding min/max and averaging remaining
// values
float process_sensor_values(float a, float b, float c)
{
    float min_val = fminf(a, fminf(b, c));
    float max_val = fmaxf(a, fmaxf(b, c));

    // Calculate sum excluding min and max values (effectively gets middle values)
    float avg = a + b + c - min_val - max_val;

    // Return average of the three values
    return avg;
}
// Get temperature task
TaskHandle_t get_temperature_handle = NULL;
void get_temperature(void* pvParameter)
{
    // Array of DS18B20 addresses
    uint8_t ds18b20_address[SENSOR_COUNT][8] = {
        // Left temperature sensor
        { 0x28, 0xf5, 0x48, 0x16, 0x00, 0x00, 0x00, 0x61 },
        // Right temperature sensor
        { 0x28, 0x1c, 0xc1, 0x11, 0x00, 0x00, 0x00, 0x60 },
        // ...
    };
    // DS18B20 initialization
    OneWire::DS18B20 onewire_pin { GPIO_NUM_12 };

    // Uncomment this to read ROM address of DS18B20, one sensor at a time
    // onewire_pin.readROM();

    SensorData_t sensor_data = {};

    // Circular buffers to store last 3 readings for each sensor (2 sensors)
    float sensor_values[2][3] = {};
    // Index pointers for circular buffers (0-2 for each sensor)
    uint8_t value_index[2] = { 0 };

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        for (uint8_t i = 0; i < SENSOR_COUNT; i++) {
            sensor_data.sensor_id = i;
            vTaskDelay(pdMS_TO_TICKS(2000));

            // Variable to store new temperature reading
            float new_temp;

            onewire_pin.get_temp(ds18b20_address[i], new_temp);

            ESP_LOGI("DS18B20", "Temperature %d: %.2f", i, sensor_data.temperature);

            // Store new reading in circular buffer
            sensor_values[i][value_index[i]] = new_temp;

            // Update circular buffer index (wraps around at 3)
            value_index[i] = (value_index[i] + 1) % 3;

            // When we've collected 3 readings (index wraps back to 0)
            if (value_index[i] == 0) {
                // Process the three readings to filter out outliers
                float filtered_temp = process_sensor_values(sensor_values[i][0], // Oldest reading
                    sensor_values[i][1], // Middle reading
                    sensor_values[i][2] // Newest reading
                );

                // Prepare data structure for queue
                sensor_data.sensor_id = i;
                sensor_data.temperature = filtered_temp;

                // Send to mqtt task
                if (xQueueSend(temperature_queue, &sensor_data, portMAX_DELAY) != pdPASS) {
                    ESP_LOGE("DS18B20", "Failed to send data to queue from main.cpp");
                }
                // Send to fan control task
                if (xQueueSend(temperature_queue_PWM, &sensor_data, portMAX_DELAY) != pdPASS) {
                    ESP_LOGE("DS18B20", "Failed to send data to queue from main.cpp");
                }
            }
        }
    }
}

TaskHandle_t ota_update_handle = NULL;
void ota_update(void* pvParameter)
{
    Ota_NS::OtaParams* params = static_cast<Ota_NS::OtaParams*>(pvParameter);
    if (params) {
        Ota_NS::Ota ota(*params);
        ota.start_update();
    }
}

// Mqtt task. Keep under other tasks for access handles
TaskHandle_t mqtt_connection_handle = NULL;
void mqtt_connection(void* pvParameter)
{
    // TaskHandle_t tasks_handles[] = { http_server_handle, get_temperature_handle
    // };
    Mqtt_NS::Mqtt mqtt(common_event_group, temperature_queue, duty_percent_queue);
    for (;;) {
        mqtt.publish();
        mqtt.connection_watcher();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
// ===================== Debugging =========================================
// Check stack memory usage
void checkStackUsage(void* pvParameter)
{
    {
        for (;;) {
            UBaseType_t highWaterMark = uxTaskGetStackHighWaterMark(http_server_handle);
            // Warning for visual difference
            ESP_LOGE("StackMonitor", "High Water Mark:%u", highWaterMark);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

// Print free heap memory
void printHeapInfo()
{
    uint32_t freeHeap = esp_get_free_heap_size();
    uint32_t freeHeapMin = esp_get_minimum_free_heap_size();
    // Warning for visual difference
    ESP_LOGE("HeapMonitor", "Free Heap Size: %u bytes", freeHeap);
    ESP_LOGE("HeapMonitor", "Minimal Heap Size: %u bytes", freeHeapMin);
}
void heapMonitor(void* pvParameter)
{
    for (;;) {
        printHeapInfo();
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

// ============================================================================
// ============================= Main Program =================================
// ============================================================================

extern "C" void app_main(void)
{
    // =========================== Logging ====================================
    // Set log level in this scope
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("*", ESP_LOG_WARN);
    // esp_log_level_set("*", ESP_LOG_ERROR);

    // ========================= Initialization ===============================

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());

    // ======================= Tasks Looping ==================================

    xTaskCreate(&wifi_connection, "Wifi", STACK_TASK_SIZE, NULL, 5,
        &wifi_connection_handle);

    xTaskCreate(&mqtt_connection, "Mqtt", STACK_TASK_SIZE, NULL, 5,
        &mqtt_connection_handle);

    xTaskCreate(&get_temperature, "Temperature", STACK_TASK_SIZE, NULL, 5,
        &get_temperature_handle);

    xTaskCreate(&fan_control, "FanControl", STACK_TASK_SIZE, NULL, 5, NULL);

    // Debug tasks
    // xTaskCreate(checkStackUsage, "CheckStack", STACK_TASK_SIZE, NULL, 5, NULL);
    xTaskCreate(&heapMonitor, "HeapMonitor", 2048, NULL, 5, NULL);
}
