#pragma once

// Copy this file to Secrets.h (same folder) and fill in your local values.
// Secrets.h is listed in .gitignore and must never be committed.

#define WIFI_PRIMARY_SSID_VALUE        "your-hotspot-ssid"
#define WIFI_PRIMARY_PASSWORD_VALUE    "your-hotspot-password"

#define WIFI_FALLBACK_SSID_VALUE       "your-fallback-ssid"
#define WIFI_FALLBACK_PASSWORD_VALUE   "your-fallback-password"

// Broker used on the fallback network. On the primary hotspot the gateway IP is used.
#define MQTT_FALLBACK_BROKER_VALUE     "192.168.x.x"

// Leave both empty when the broker does not use authentication.
#define MQTT_USERNAME_VALUE            ""
#define MQTT_PASSWORD_VALUE            ""
