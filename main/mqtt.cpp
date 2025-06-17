#include "mqtt.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_system.h"
#include "mqtt_device.h"
#include "portmacro.h"
#include "projdefs.h"
#include <cstddef>

std::string get_current_ip()
{
    // Ждём, пока WiFi получит IP (макс. 10 секунд)

    tcpip_adapter_ip_info_t ip_info;
    if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info) == ESP_OK) {
        // Проверяем, что IP не 0.0.0.0
        if (ip_info.ip.addr != 0) {
            char ip_str[16];
            snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip4_addr1(&ip_info.ip),
                ip4_addr2(&ip_info.ip), ip4_addr3(&ip_info.ip),
                ip4_addr4(&ip_info.ip));
            return std::string(ip_str);
        }
    }
    return "0.0.0.0"; // Возвращаем 0.0.0.0, если IP не получен
}
namespace Mqtt_NS {

// Static variables
esp_mqtt_client_config_t Mqtt::mqtt_cfg {};

// Constructor
Mqtt::Mqtt(EventGroupHandle_t& common_event_group,
    QueueHandle_t& temperature_queue, QueueHandle_t& percent_queue)
    : _state(state_m::NOT_INITIALISED)
    , _common_event_group(&common_event_group)
    , _sensor_queue(&temperature_queue)
    , _percent_queue(&percent_queue)
    , _mdns_mqtt_server({})
{
    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
        WIFI_EVENT_STA_DISCONNECTED,
        &Mqtt::_disconnect_handler, this));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
        &Mqtt::_connect_handler, this));
}

Mqtt::~Mqtt(void)
{
    if (client != nullptr) {
        stop();
    }
}

void Mqtt::_disconnect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "Disconnect event occured");
    Mqtt* self = static_cast<Mqtt*>(arg);
    if (self->client == NULL) {
        return;
    }
    self->stop();
}

void Mqtt::stop()
{
    if (client == nullptr) {
        ESP_LOGI(TAG, "MQTT client already stopped");
        return;
    }

    ESP_LOGI(TAG, "Stoping mqtt client...");
    esp_mqtt_client_destroy(client);
    client = nullptr;

    // ESP_ERROR_CHECK(esp_event_handler_unregister(
    //     WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &Mqtt::_disconnect_handler));
    // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
    //     &Mqtt::_connect_handler))
    _state = state_m::NOT_INITIALISED;
}

bool Mqtt::find_mqtt_server(MdnsMqttServer_t& mqtt_server)
{

    // Initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    ESP_LOGI(TAG_mDNS, "Querying for MQTT servers...");
    mdns_result_t* results = NULL;
    esp_err_t err = mdns_query_ptr("_mqtt", "_tcp", 3000, 20, &results);

    if (err != ESP_OK || results == NULL) {
        ESP_LOGW(TAG_mDNS, "Failed to find MQTT servers");
        mdns_query_results_free(results);
        return false;
    }

    // Check results for errors
    if (results->hostname == nullptr || results->addr == nullptr) {
        mdns_query_results_free(results);
        return false;
    }

    if (results->instance_name) {
        printf("  PTR : %s\n", results->instance_name);
    }
    if (results->hostname) {
        printf("  SRV : %s.local:%u\n", results->hostname, results->port);
    }

    snprintf(mqtt_server.hostname, sizeof(mqtt_server.hostname), "%s.local",
        results->hostname);
    ESP_LOGI(TAG_mDNS, "Found MQTT server: %s", mqtt_server.hostname);

    snprintf(mqtt_server.ip, sizeof(mqtt_server.ip), "%s",
        ip4addr_ntoa(&(results->addr->addr.u_addr.ip4)));
    ESP_LOGI(TAG_mDNS, "Found MQTT server IP: %s", mqtt_server.ip);

    snprintf(mqtt_server.full_proto, sizeof(mqtt_server.full_proto), "mqtt://%s",
        mqtt_server.ip); // "mqtt://192.168.111.222"

    mqtt_server.port = results->port;
    ESP_LOGI(TAG_mDNS, "Found MQTT server port: %d", mqtt_server.port);

    // Free results
    if (results) {
        mdns_query_results_free(results);
    }

    mdns_free();
    return true;
}

void Mqtt::_connect_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    Mqtt* self = static_cast<Mqtt*>(arg);
    if (self->client == NULL) {
        ESP_LOGI(TAG, "Initializing client...");
        self->init();
    }
}

// Initialisation mqtt client
void Mqtt::init(void)
{
    ESP_LOGI(TAG, "Init method");
    if (_state == state_m::INITIALISED) {
        return;
    }

    // Create queue set
    if (_queue_set != nullptr) {
        vQueueDelete(_queue_set);
    }

    _queue_set = xQueueCreateSet(2);
    xQueueAddToSet(*_sensor_queue, _queue_set);
    xQueueAddToSet(*_percent_queue, _queue_set);

    if (find_mqtt_server(_mdns_mqtt_server)) {
        // Connect through mDNS
        mqtt_cfg.uri = _mdns_mqtt_server.full_proto;
        mqtt_cfg.port = _mdns_mqtt_server.port;
    } else {
        mqtt_cfg.uri = CONFIG_BROKER_URL;
        mqtt_cfg.port = CONFIG_BROKER_PORT;
    }
    mqtt_cfg.client_id = CONFIG_CLIENT_ID;
    mqtt_cfg.username = MQTT_USER;
    mqtt_cfg.password = MQTT_PASSWORD;
    mqtt_cfg.keepalive = CONFIG_MQTT_KEEP_ALIVE;
    mqtt_cfg.protocol_ver = MQTT_PROTOCOL_V_3_1_1;
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_err_t ret = esp_mqtt_client_register_event(client, MQTT_EVENT_ANY,
        mqtt_event_handler, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register event handler: %s", esp_err_to_name(ret));
    }
    _state = state_m::INITIALISED;
    ESP_LOGI(TAG, "Client initialiased");

    // Call start
    start();
}

void Mqtt::start()
{
    if (client != nullptr and _state == state_m::INITIALISED) {
        esp_mqtt_client_start(client);
        _state = state_m::STARTED;
        ESP_LOGI(TAG, "Client started");
        // Watcher for connection
        // connection_watcher();
    }
}

void Mqtt::connection_watcher()
{
    EventBits_t bit = xEventGroupWaitBits(
        *_common_event_group, _mqtt_disconnect_bit, pdTRUE, pdFALSE, 0);

    if (bit & _mqtt_disconnect_bit) {
        bit = 0;
        stop();
        init();
    }
}

// Events handler
void Mqtt::mqtt_event_handler(void* handler_args, esp_event_base_t event_base,
    int32_t event_id, void* event_data)
{
    Mqtt* instance = static_cast<Mqtt*>(handler_args);
    if (instance) {
        instance->mqtt_event_handler_cb(
            static_cast<esp_mqtt_event_handle_t>(event_data));
    }
}

// Evens callback
esp_err_t Mqtt::mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {

    case MQTT_EVENT_CONNECTED:
        if (_state != state_m::CONNECTED) {
            // Set mqtt connection bit
            xEventGroupSetBits(*_common_event_group, _mqtt_connect_bit);
        }

        ESP_LOGI(TAG, "Connected to MQTT broker at %s:%d", mqtt_cfg.uri,
            mqtt_cfg.port);
        ESP_LOGI(TAG, "Login - \"%s\" password - \"%s\"", mqtt_cfg.username,
            mqtt_cfg.password);
        _state = state_m::CONNECTED;
        _connection_retry = 0;

        // Clear queues
        xQueueReset(*_sensor_queue);
        xQueueReset(*_percent_queue);

        // Device left HDD
        esp_mqtt_client_publish(event->client, topic_left.c_str(),
            (get_device_json() + msg_left).c_str(), 0, 1, 1);
        // Device right HDD
        esp_mqtt_client_publish(event->client, topic_right.c_str(),
            (get_device_json() + msg_right).c_str(), 0, 1, 1);
        // Device fan
        esp_mqtt_client_publish(event->client, topic_fan.c_str(),
            (get_device_json() + msg_fan).c_str(), 0, 1, 1);

        // Subscribe to command topic
        esp_mqtt_client_subscribe(event->client, command_topic.c_str(), 0);
        ESP_LOGI(TAG, "Subscribed to command topic %s", command_topic.c_str());
        break;

    case MQTT_EVENT_DISCONNECTED:

        // Retry connection 20 times about 3 minutes
        if (_connection_retry > MAX_CONNECTION_RETRIES) {
            ESP_LOGW(
                TAG,
                "Client was not able to connect to MQTT broker. Deinitializing...");
            _connection_retry = 0;
            xEventGroupSetBits(*_common_event_group, _mqtt_disconnect_bit);
            break;
        }
        ESP_LOGI(TAG, "Trying to connect MQTT broker. Try # %d",
            _connection_retry + 1);
        if (_state == state_m::CONNECTED) {
            ESP_LOGI(TAG, "Disconnected from MQTT broker at %s:%d", mqtt_cfg.uri,
                mqtt_cfg.port);
            _state = state_m::DISCONNECTED;
        }
        _connection_retry++;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        break;
    case MQTT_EVENT_DATA:

        ESP_LOGI(TAG, "Command received from topic - %s", event->topic);
        ESP_LOGI(TAG, "Command received - %s", event->data);

        if (strncmp(event->topic, command_topic.c_str(), event->data_len) == 0) {

            if (strncmp(event->data, "ENABLE_HTTP", event->data_len) == 0) {
                is_http_running = true;
                vTaskDelete(get_temperature_handle);
                vTaskDelay(pdMS_TO_TICKS(100));
                xTaskCreate(&http_server, "HTTP Server", STACK_TASK_SIZE * 2, NULL, 5,
                    &http_server_handle);

            } else if (strncmp(event->data, "DISABLE_HTTP", event->data_len) == 0) {
                // Restart because OTA conflicting after http server disable
                esp_restart();

            } else if (strncmp(event->data, "RESTART", event->data_len) == 0) {
                esp_restart();

            } else if (strncmp(event->data, "UPDATE", event->data_len) == 0) {
                is_http_running = true;
                vTaskDelete(get_temperature_handle);
                vTaskDelay(pdMS_TO_TICKS(100));
                Ota_NS::OtaParams* params = new Ota_NS::OtaParams;
                params->firmware_url = "http://192.168.8.167:8000/HDDStation.bin";
                xTaskCreate(&ota_update, "OTA_Update", STACK_TASK_SIZE * 4, params, 5,
                    NULL);
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    return ESP_OK;
}
// Start MQTT client
void Mqtt::publish()
{
    // Publish sensor data
    SensorData_t sensor_data {};
    uint8_t percent {};

    // xEventGroupWaitBits(*_common_event_group, _mqtt_connect_bit, pdTRUE,
    // pdFALSE,
    //     portMAX_DELAY);
    if (_state != state_m::CONNECTED) {
        return;
    }

    QueueSetMemberHandle_t activate_handle = xQueueSelectFromSet(_queue_set, 0);
    if (activate_handle == nullptr) {
        return;
    }

    if (activate_handle == *_sensor_queue) {
        if (xQueueReceive(*_sensor_queue, &sensor_data, portMAX_DELAY) == pdTRUE) {
            if (sensor_data.temperature == 0.0f && sensor_data.sensor_id == 0) {
                ESP_LOGW(TAG, "Received zero value from sensor %d",
                    sensor_data.sensor_id);
                return; // skip sending
            }
            char msg[12]; // buffer for message
            snprintf(msg, sizeof(msg), "%.2f", sensor_data.temperature);

            char topic[64]; // buffer for topic
            snprintf(topic, sizeof(topic),
                "homeassistant/sensor/HDDdock/temp_%d/state",
                sensor_data.sensor_id);

            ESP_LOGI(TAG, "Temperature from MQTT: %s %s", msg, topic);
            esp_mqtt_client_publish(client, topic, msg, 0, 0, 0);
        } else {
            ESP_LOGE(TAG, "Failed to receive data from sensor queue");
        }

    } else if (activate_handle == *_percent_queue) {
        if (xQueueReceive(*_percent_queue, &percent, portMAX_DELAY) == pdTRUE) {

            char msg[10]; // buffer for message
            snprintf(msg, sizeof(msg), "%d", percent);

            char topic[50]; // buffer for topic
            snprintf(topic, sizeof(topic), "homeassistant/sensor/HDDdock/fan/state");

            ESP_LOGI(TAG, "Percent from MQTT: %s %s", msg, topic);
            esp_mqtt_client_publish(client, topic, msg, 0, 0, 0);
        } else {
            ESP_LOGE(TAG, "Failed to receive data from percent queue");
        }
    }
}

} // namespace Mqtt_NS
