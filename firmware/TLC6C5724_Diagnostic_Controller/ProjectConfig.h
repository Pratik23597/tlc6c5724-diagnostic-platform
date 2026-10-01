#pragma once

#include <Arduino.h>

// Network credentials are kept outside the sketch.
// Copy Secrets.example.h to Secrets.h and fill in your local values.
#if defined(__has_include)
  #if __has_include("Secrets.h")
    #include "Secrets.h"
  #endif
#endif

#ifndef WIFI_FALLBACK_SSID_VALUE
  #define WIFI_FALLBACK_SSID_VALUE ""
#endif
#ifndef WIFI_FALLBACK_PASSWORD_VALUE
  #define WIFI_FALLBACK_PASSWORD_VALUE ""
#endif
#ifndef WIFI_PRIMARY_SSID_VALUE
  #define WIFI_PRIMARY_SSID_VALUE ""
#endif
#ifndef WIFI_PRIMARY_PASSWORD_VALUE
  #define WIFI_PRIMARY_PASSWORD_VALUE ""
#endif
#ifndef MQTT_FALLBACK_BROKER_VALUE
  #define MQTT_FALLBACK_BROKER_VALUE ""
#endif
#ifndef MQTT_USERNAME_VALUE
  #define MQTT_USERNAME_VALUE ""
#endif
#ifndef MQTT_PASSWORD_VALUE
  #define MQTT_PASSWORD_VALUE ""
#endif

static const char WIFI_FALLBACK_SSID[] = WIFI_FALLBACK_SSID_VALUE;
static const char WIFI_FALLBACK_PASSWORD[] = WIFI_FALLBACK_PASSWORD_VALUE;
static const char WIFI_PRIMARY_SSID[] = WIFI_PRIMARY_SSID_VALUE;
static const char WIFI_PRIMARY_PASSWORD[] = WIFI_PRIMARY_PASSWORD_VALUE;

static const char MQTT_FALLBACK_BROKER[] = MQTT_FALLBACK_BROKER_VALUE;
static const char MQTT_USERNAME[] = MQTT_USERNAME_VALUE;
static const char MQTT_PASSWORD[] = MQTT_PASSWORD_VALUE;

constexpr uint16_t MQTT_PORT = 1883;

static const char MQTT_DEVICE_ID[] = "tlc6c5724-device01";

static const char MQTT_TOPIC_AVAILABILITY[] = "tlc6c5724/device01/availability";
static const char MQTT_TOPIC_HEARTBEAT[] = "tlc6c5724/device01/heartbeat";
static const char MQTT_TOPIC_ERR[] = "tlc6c5724/device01/err";
static const char MQTT_TOPIC_CHANNEL_SCAN[] = "tlc6c5724/device01/channel-scan";
static const char MQTT_TOPIC_APS[] = "tlc6c5724/device01/aps";
static const char MQTT_TOPIC_DEVICE_STATUS[] = "tlc6c5724/device01/device-status";
static const char MQTT_TOPIC_LOD_LSD_SELF_TEST[] = "tlc6c5724/device01/lod-lsd-self-test";
static const char MQTT_TOPIC_NEG_GCLK[] = "tlc6c5724/device01/neg-gclk";
static const char MQTT_TOPIC_ERROR_CLEAR[] = "tlc6c5724/device01/error-clear";
static const char MQTT_TOPIC_COMMAND[] = "tlc6c5724/device01/command";
static const char MQTT_TOPIC_COMMAND_STATUS[] = "tlc6c5724/device01/command-status";
static const char MQTT_TOPIC_CAPABILITIES[] = "tlc6c5724/device01/capabilities";
static const char MQTT_TOPIC_CHANNEL_CONTROL[] = "tlc6c5724/device01/channel-control";
static const char MQTT_TOPIC_AUTOMATIC_TEST[] = "tlc6c5724/device01/automatic-test";

constexpr uint32_t WIFI_PROFILE_RECOVERY_MS = 12000;
constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 10000;
constexpr uint32_t MQTT_RETRY_INTERVAL_MS = 5000;
constexpr uint32_t MQTT_HEARTBEAT_INTERVAL_MS = 30000;
