#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"           // IWYU pragma: keep
#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "mdns.h" // IWYU pragma: keep
#include "mqtt_client.h"
#include "mqtt_device.h" // IWYU pragma: keep
#include "nvs.h"
#include "nvs_flash.h" // IWYU pragma: keep
#include "ota.h"       // IWYU pragma: keep
#include "secrets.h"   // IWYU pragma: keep
#include <cstdint>

extern "C" {
void http_server(void *pvParameter);
extern TaskHandle_t http_server_handle;
void get_temperature(void *pvParameter);
extern TaskHandle_t get_temperature_handle;
void ota_update(void *pvParameter);
extern TaskHandle_t ota_update_handle;

extern volatile bool is_http_running;
extern uint16_t STACK_TASK_SIZE;
}

typedef struct {
  uint8_t sensor_id; // ID sensor
  float temperature; // Temperature
} SensorData_t;

typedef struct {
  char hostname[60];   // hostname
  char ip[16];         // IPv4 (example "192.168.111.222")
  char full_proto[25]; // mqtt://192.168.111.222
  uint16_t port;       // Port MQTT (1883, 8883 и т.д.)
} MdnsMqttServer_t;

namespace Mqtt_NS {

class Mqtt {
private:
  enum class state_m {
    NOT_INITIALISED,
    INITIALISED,
    STARTED,
    STOPPED,
    CONNECTED,
    DISCONNECTED,
    // ERROR
  };
  uint8_t _connection_retry;
  state_m _state;
  esp_mqtt_client_handle_t client = NULL;
  static esp_mqtt_client_config_t mqtt_cfg;
  /*
      wifi 0 bit - connected
      wifi 1 bit - disconnected
      wifi 2 bit - got ip
      wifi 3 bit - lost ip
      wifi 4 bit - reserved
      mqtt 5 bit - connected
      mqtt 6 bit - disconnected
      mqtt 7 bit - subscribed
      mqtt 8 bit - unsubscribed
      mqtt 9 bit - reserved
  */
  const uint8_t _mqtt_connect_bit{BIT5};
  const uint8_t _mqtt_disconnect_bit{BIT6};
  const uint8_t _mqtt_subscribed{BIT7};
  const uint16_t _mqtt_unsubscribed{BIT8};

  // Handle WI-FI event
  static void _connect_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data);
  static void _disconnect_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data);

  // Handle MQTT event
  static void mqtt_event_handler(void *handler_args,
                                 esp_event_base_t event_base, int32_t event_id,
                                 void *event_data);
  esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);

  QueueSetHandle_t _queue_set = nullptr;
  EventGroupHandle_t *_common_event_group;
  QueueHandle_t *_sensor_queue;
  QueueHandle_t *_percent_queue;

  MdnsMqttServer_t _mdns_mqtt_server;
  state_m _mdns_interface_state{state_m::NOT_INITIALISED};

  // NVS
  static Nvs_NS::Nvs *_nvs;

public:
  Mqtt(EventGroupHandle_t &common_event_group, QueueHandle_t &temperature_queue,
       QueueHandle_t &percent_queue, Nvs_NS::Nvs *nvs);
  ~Mqtt(void);
  bool find_mqtt_server(MdnsMqttServer_t &mqtt_server);
  void connection_watcher();
  void init(void);
  void publish(void);
  void stop();
  void start();

  std::string get_current_ip();

  constexpr static const char *TAG = "MQTT";
  constexpr static const char *TAG_mDNS = "mDNS";

  static constexpr uint8_t MAX_CONNECTION_RETRIES = 3;
  static constexpr uint32_t MDNS_QUERY_TIMEOUT_MS = 10000;
};

} // namespace Mqtt_NS
