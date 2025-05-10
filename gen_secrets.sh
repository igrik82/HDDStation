#!/bin/bash

cat <<EOF >main/secrets.h
#pragma once

#define WIFI_SSID "$WIFI_SSID"
#define WIFI_PASSWORD "$WIFI_PASSWORD"
#define MQTT_USER "$MQTT_USER"
#define MQTT_PASSWORD "$MQTT_PASSWORD"

EOF

printf "Updated secret.h with \nWIFI_SSID=%s\nWIFI_PASSWORD=%s\nMQTT_USER=%s\nMQTT_PASSWORD=%s\n" \
  "${WIFI_SSID}" "${WIFI_PASSWORD}" "${MQTT_USER}" "${MQTT_PASSWORD}"
