#pragma once
#include <string>

// Device
const std::string device = R"({
  "device": {
    "name": "HDD Station",
    "model": "HDD Station",
    "ids": "DockHDD24D7EB118208",
    "mf": "Игорь Смоляков",
    "sw": "2.00",
    "hw": "1.01",
    "cu": "http://192.168.88.13"
  },)";

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
