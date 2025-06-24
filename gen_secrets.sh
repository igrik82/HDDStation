#!/bin/bash

cat <<EOF >main/secrets.h
#pragma once

#define STORAGE_SPACE "$STORAGE_SPACE"

#define MIN_HDD_TEMP_KEY "$MIN_HDD_TEMP_KEY"
#define MIN_HDD_TEMP $MIN_HDD_TEMP

#define MAX_HDD_TEMP_KEY "$MAX_HDD_TEMP_KEY"
#define MAX_HDD_TEMP $MAX_HDD_TEMP

#define FREQUENCY_KEY "$FREQUENCY_KEY"
#define FREQUENCY $FREQUENCY

#define SENSOR_0_KEY "$SENSOR_0_KEY"
#define SENSOR_0 $SENSOR_0

#define SENSOR_1_KEY "$SENSOR_1_KEY"
#define SENSOR_1 $SENSOR_1

#define WIFI_SSID_KEY "$WIFI_SSID_KEY"
#define WIFI_SSID "$WIFI_SSID"

#define WIFI_PASSWORD_KEY "$WIFI_PASSWORD_KEY"
#define WIFI_PASSWORD "$WIFI_PASSWORD"

#define MQTT_HOST_KEY "$MQTT_HOST_KEY"
#define MQTT_HOST "$MQTT_HOST"

#define MQTT_PORT_KEY "$MQTT_PORT_KEY"
#define MQTT_PORT $MQTT_PORT

#define MQTT_USER_KEY "$MQTT_USER_KEY"
#define MQTT_USER "$MQTT_USER"

#define MQTT_PASSWORD_KEY "$MQTT_PASSWORD_KEY"
#define MQTT_PASSWORD "$MQTT_PASSWORD"

EOF

printf "Updated secret.h:
%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n" \
    "NVS storage" "${STORAGE_SPACE}" \
    "${MIN_HDD_TEMP_KEY}" "${MIN_HDD_TEMP}" \
    "${MAX_HDD_TEMP_KEY}" "${MAX_HDD_TEMP}" \
    "${FREQUENCY_KEY}" "${FREQUENCY}" \
    "${SENSOR_0_KEY}" "${SENSOR_0}" \
    "${SENSOR_1_KEY}" "${SENSOR_1}" \
    "${WIFI_SSID_KEY}" "${WIFI_SSID}" \
    "${WIFI_PASSWORD_KEY}" "${WIFI_PASSWORD}" \
    "${MQTT_HOST_KEY}" "${MQTT_HOST}" \
    "${MQTT_PORT_KEY}" "${MQTT_PORT}" \
    "${MQTT_USER_KEY}" "${MQTT_USER}" \
    "${MQTT_PASSWORD_KEY}" "${MQTT_PASSWORD}"
