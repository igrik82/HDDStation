# HDDStation - HDD Temperature Monitoring and Fan Control

## Overview

HDDStation is an embedded project designed to monitor the temperature of two Hard Disk Drives (HDDs) and control a cooling fan to maintain optimal operating temperatures. It is built using the ESP-IDF framework for Espressif microcontrollers (ESP32/ESP8266).

The device reads temperature data from two DS18B20 sensors, processes the readings to filter out anomalies, and adjusts the speed of a PWM-controlled fan accordingly.

It features Wi-Fi connectivity, an integrated web server for configuration, and MQTT support for integration with home automation systems like Home Assistant. The firmware can be updated remotely via Over-the-Air (OTA) updates.

## Features

*   **Dual Temperature Sensing:** Monitors two separate locations using DS18B20 temperature sensors.
*   **PWM Fan Control:** Automatically adjusts fan speed based on configurable temperature thresholds.
*   **Wi-Fi Connectivity:** Connects to your local Wi-Fi network.
*   **Web Interface:** An integrated web server allows you to:
    *   View current settings.
    *   Configure Wi-Fi credentials.
    *   Set fan control parameters (min/max temperature, PWM frequency).
    *   Configure MQTT broker settings.
    *   Reboot the device.
*   **MQTT Integration:**
    *   Publishes temperature and fan speed data to an MQTT broker.
    *   Integrates with Home Assistant using MQTT discovery.
    *   Supports commands via MQTT to enable/disable the web server, trigger OTA updates, or restart the device.
    *   Automatically discovers the MQTT broker on the network using mDNS.
*   **OTA Updates:** Supports remote firmware updates over the air.
*   **Persistent Configuration:** Saves all settings to Non-Volatile Storage (NVS).

## MQTT Integration

The device integrates seamlessly with Home Assistant via MQTT discovery.

### Published Topics

*   **Temperature Sensor 1:** `homeassistant/sensor/HDDdock/temp_0/state`
*   **Temperature Sensor 2:** `homeassistant/sensor/HDDdock/temp_1/state`
*   **Fan Speed (%):** `homeassistant/sensor/HDDdock/fan/state`

### Command Topic

*   **Commands:** `homeassistant/switch/HDDdock/command`

    *   `ENABLE_HTTP`: Starts the web server.
    *   `DISABLE_HTTP`: Stops the web server and reboots the device.
    *   `RESTART`: Reboots the device.
    *   `UPDATE`: Triggers an OTA firmware update.

## Building and Flashing

### Prerequisites

*   ESP8266 RTOS SDK

### Configuration

1.  **Set up Environment Variables:**
    Create a `.env` file in the project root or export the following environment variables required for building the firmware. These are used by the `gen_secrets.sh` script to generate the `main/secrets.h` file.

    ```bash
    export STORAGE_SPACE="storage"
    export MIN_HDD_TEMP_KEY="min_hdd_temp"
    export MIN_HDD_TEMP=30
    export MAX_HDD_TEMP_KEY="max_hdd_temp"
    export MAX_HDD_TEMP=45
    export FREQUENCY_KEY="frequency"
    export FREQUENCY=25000
    export SENSOR_0_KEY="sensor_0_corr"
    export SENSOR_0=0.0
    export SENSOR_1_KEY="sensor_1_corr"
    export SENSOR_1=0.0
    export WIFI_SSID_KEY="wifi_ssid"
    export WIFI_SSID="YourSSID"
    export WIFI_PASSWORD_KEY="wifi_password"
    export WIFI_PASSWORD="YourPassword"
    export MQTT_HOST_KEY="mqtt_host"
    export MQTT_HOST="192.168.1.100"
    export MQTT_PORT_KEY="mqtt_port"
    export MQTT_PORT=1883
    export MQTT_USER_KEY="mqtt_user"
    export MQTT_USER="your_mqtt_user"
    export MQTT_PASSWORD_KEY="mqtt_password"
    export MQTT_PASSWORD="your_mqtt_password"
    ```

2.  **Generate Secrets:**
    Run the script to generate the header file with your secrets:

    ```sh
    ./gen_secrets.sh
    ```

### Build and Flash
1. **Configure project**
    ```sh
    make menuconfig
    ```

1.  **Build the Project:**
    ```sh
    make 
    ```

2.  **Flash the Firmware:**
    Connect your device and flash the firmware:
    ```sh
    make flash
    ```
    (Replace `/dev/ttyUSB0` with your device's serial port).

4.  **Monitor Output:**
    ```sh
    make flash monitor
    ```

## Web Interface

Once the device is connected to your Wi-Fi network, you can access the web interface by navigating to its IP address in a web browser. The IP address will be printed in the serial monitor upon connection.

The web interface provides a simple way to configure all device settings without needing to re-flash the firmware.
