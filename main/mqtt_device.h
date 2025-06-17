#pragma once
#include "mqtt.h"
#include "version.h"
#include <string>
std::string get_current_ip();

// Макросы для преобразования версий в строку
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define SW_VERSION          \
    TOSTRING(VERSION_MAJOR) \
    "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH)

inline std::string get_device_json()
{
    std::string ip = get_current_ip();
    return R"({
  "device": {
    "name": "HDD Station",
    "model": "HDD Station",
    "ids": "DockHDD24D7EB118208",
    "mf": "Игорь Смоляков",
    "sw": ")" SW_VERSION R"(",
    "cu": "http://)"
        + ip + R"("
  })";
}

// Device left HDD
const std::string topic_left = R"(homeassistant/sensor/HDDdock_temp_left/config)";
const std::string msg_left = R"(
  "name": "Left HDD",
  "deve_cla": "temperature",
  "stat_t": "homeassistant/sensor/HDDdock/temp_0/state",
  "uniq_id": "DockHDD_temp_left",
  "icon": "mdi:harddisk",
  "unit_of_meas": "°C"
})";

// Device right HDD
const std::string topic_right = R"(homeassistant/sensor/HDDdock_temp_right/config)";
const std::string msg_right = R"(
  "name": "Right HDD",
  "deve_cla": "temperature",
  "stat_t": "homeassistant/sensor/HDDdock/temp_1/state",
  "uniq_id": "DockHDD_temp_right",
  "icon": "mdi:harddisk",
  "unit_of_meas": "°C"
 })";

// Device fan
const std::string topic_fan = R"(homeassistant/sensor/HDDdock_fan/config)";
const std::string msg_fan = R"(
    "name": "Fan HDD",
  "deve_cla": "power_factor",
  "stat_t": "homeassistant/sensor/HDDdock/fan/state",
  "uniq_id": "DockHDD_fan",
  "icon": "mdi:fan",
  "unit_of_meas": "%"
 })";

const std::string command_topic = R"(homeassistant/sensor/HDDdock_commands)";
