//   Xiaomi random MACs
//   FC:19:99:A4:B9:08
//   FC:19:99:A4:9C:A4
//   FC:19:99:A4:5C:24
//   FC:19:99:A4:46:F2
//   FC:19:99:A4:5E:15
//   FC:19:99:A4:67:C8
//   FC:19:99:A4:CA:5A
//   FC:19:99:A4:2D:E2
//   FC:19:99:A4:2E:4B
//   FC:19:99:A4:28:FC
#pragma once

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_event_base.h"
#include "esp_log.h" // IWYU pragma: keep
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "event_groups.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h" // IWYU pragma: keep
#include "secrets.h" // IWYU pragma: keep
#include <cstring>
#include <string>

// Power mode Wi-FI
#if CONFIG_WIFI_PS_NONE
#define POWER_MODE WIFI_PS_NONE
#elif CONFIG_WIFI_PS_MIN_MODEM
#define POWER_MODE WIFI_PS_MIN_MODEM
#elif CONFIG_WIFI_PS_MAX_MODEM
#define POWER_MODE WIFI_PS_MAX_MODEM
#endif

namespace Wifi_NS {
class Wifi {

private:
    enum class state_w {
        NOT_INITIALISED,
        INITIALISED,
        READY_TO_CONNECT,
        CONNECTING,
        WAITING_FOR_IP,
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

    const gpio_num_t _led { (gpio_num_t)CONFIG_LED_GPIO };

// Custom mac addr
#if CONFIG_CUSTOM_MAC
    const uint8_t _mac_addr[6] { 0xFC, 0x19, 0x99, 0xA4, 0xB9, 0x08 };
#endif
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

    static wifi_init_config_t _wifi_init_config;
    static wifi_config_t wifi_config;
    static state_w _state;

    static std::string ip;

    void _led_blink(void);
    esp_err_t _led_on(void);
    esp_err_t _led_off(void);

    // static EventGroupHandle_t _wifi_event_group;
    static void _event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);
    static void _wifi_event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);
    static void _ip_event_handler(void* arg, esp_event_base_t event_base,
        int32_t event_id, void* event_data);
    static esp_err_t _init_wifi(void);

public:
    Wifi(void);
    ~Wifi(void) = default;
    esp_err_t start(void);
    esp_err_t stop(void);
    static std::string get_ip(void) { return ip; }
    constexpr static const char* TAG = "Wi-Fi";
};

} // namespace Wifi_NS
