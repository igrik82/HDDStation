#!/bin/bash

cat <<EOF >main/secrets.h
#pragma once

#define WIFI_SSID_KEY "$WIFI_SSID_KEY"
#define WIFI_SSID "$WIFI_SSID"

#define WIFI_PASSWORD_KEY "$WIFI_PASSWORD_KEY"
#define WIFI_PASSWORD "$WIFI_PASSWORD"

#define MQTT_USER_KEY "$MQTT_USER_KEY"
#define MQTT_USER "$MQTT_USER"

#define MQTT_PASSWORD_KEY "$MQTT_PASSWORD_KEY"
#define MQTT_PASSWORD "$MQTT_PASSWORD"

EOF

printf "Updated secret.h:
%s ==> %s\n%s ==> %s\n%s ==> %s\n%s ==> %s\n" \
    "${WIFI_SSID_KEY}" "${WIFI_SSID}" \
    "${WIFI_PASSWORD_KEY}" "${WIFI_PASSWORD}" \
    "${MQTT_USER_KEY}" "${MQTT_USER}" \
    "${MQTT_PASSWORD_KEY}" "${MQTT_PASSWORD}"
