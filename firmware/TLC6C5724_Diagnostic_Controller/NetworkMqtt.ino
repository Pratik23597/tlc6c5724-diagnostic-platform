void beginWifiProfile(bool primaryProfile) {
  wifiTryingPrimaryProfile = primaryProfile;

  WiFi.disconnect();

  if (primaryProfile) {
    Serial.println("Trying preferred Wi-Fi profile...");
    WiFi.begin(WIFI_PRIMARY_SSID, WIFI_PRIMARY_PASSWORD);
  }
  else {
    Serial.println("Trying fallback Wi-Fi profile...");
    WiFi.begin(WIFI_FALLBACK_SSID, WIFI_FALLBACK_PASSWORD);
  }

  wifiBeginIssued = true;
  previousWifiAttemptTime = millis();
}

void beginWifiConnection() {
  Serial.println();
  Serial.println("Starting portable Wi-Fi connection...");

  WiFi.mode(WIFI_STA);

  // Profile failover is handled here so the preferred hotspot is tried first.
  WiFi.setAutoReconnect(false);

  beginWifiProfile(true);
}

void configureMqttClient() {
  // The broker is selected after Wi-Fi connects.
  mqttClient.setCallback(mqttMessageCallback);
  mqttClient.setBufferSize(1024);
  mqttClient.setKeepAlive(30);
  mqttClient.setSocketTimeout(2);
}

void configureMqttBrokerForCurrentWifi() {
  const String connectedSsid = WiFi.SSID();

  if (connectedSsid == WIFI_PRIMARY_SSID) {
    const IPAddress primaryBroker = WiFi.gatewayIP();
    mqttClient.setServer(primaryBroker, MQTT_PORT);

    Serial.println("Using broker from the preferred hotspot gateway.");
  }
  else {
    mqttClient.setServer(MQTT_FALLBACK_BROKER, MQTT_PORT);

    Serial.println("Using configured fallback MQTT broker.");
  }
}


int8_t findChannelIndexByName(
  const char* channelName
) {
  if (channelName == nullptr || strlen(channelName) != 2) {
    return -1;
  }

  const char color = static_cast<char>(
    toupper(static_cast<unsigned char>(channelName[0]))
  );

  if (channelName[1] < '0' || channelName[1] > '7') {
    return -1;
  }

  uint8_t colorOffset = 0;

  if (color == 'R') {
    colorOffset = 0;
  }
  else if (color == 'G') {
    colorOffset = 1;
  }
  else if (color == 'B') {
    colorOffset = 2;
  }
  else {
    return -1;
  }

  const uint8_t ledIndex =
    static_cast<uint8_t>(channelName[1] - '0');

  return static_cast<int8_t>(
    ledIndex * 3U + colorOffset
  );
}

RemoteCommand parseRemoteCommand(
  const char* commandText
) {
  if (
    strcmp(commandText, "device_status") == 0 ||
    strcmp(commandText, "status") == 0
  ) {
    return REMOTE_COMMAND_DEVICE_STATUS;
  }

  if (
    strcmp(commandText, "channel_scan") == 0 ||
    strcmp(commandText, "scan") == 0
  ) {
    return REMOTE_COMMAND_CHANNEL_SCAN;
  }

  if (
    strcmp(commandText, "aps") == 0 ||
    strcmp(commandText, "adjacent_pin_test") == 0
  ) {
    return REMOTE_COMMAND_APS;
  }

  if (
    strcmp(commandText, "lod_lsd") == 0 ||
    strcmp(commandText, "lod_lsd_self_test") == 0
  ) {
    return REMOTE_COMMAND_LOD_LSD;
  }

  if (
    strcmp(commandText, "neg_gclk") == 0 ||
    strcmp(commandText, "neg_gclk_test") == 0
  ) {
    return REMOTE_COMMAND_NEG_GCLK;
  }

  if (
    strcmp(commandText, "clear_errors") == 0 ||
    strcmp(commandText, "clear") == 0
  ) {
    return REMOTE_COMMAND_CLEAR_ERRORS;
  }

  if (
    strcmp(commandText, "rgb_cycle") == 0 ||
    strcmp(commandText, "rgb") == 0
  ) {
    return REMOTE_COMMAND_RGB_CYCLE;
  }

  if (
    strcmp(commandText, "rgb_phase_rotation") == 0 ||
    strcmp(commandText, "phase_rotation") == 0 ||
    strcmp(commandText, "rgb_phase") == 0
  ) {
    return REMOTE_COMMAND_RGB_PHASE_ROTATION;
  }

  if (
    strcmp(commandText, "led_chase") == 0 ||
    strcmp(commandText, "chase") == 0
  ) {
    return REMOTE_COMMAND_LED_CHASE;
  }

  if (
    strcmp(commandText, "brightness_sweep") == 0 ||
    strcmp(commandText, "sweep") == 0
  ) {
    return REMOTE_COMMAND_BRIGHTNESS_SWEEP;
  }

  if (
    strcmp(commandText, "all_white") == 0 ||
    strcmp(commandText, "white") == 0
  ) {
    return REMOTE_COMMAND_ALL_WHITE;
  }

  if (
    strcmp(commandText, "all_off") == 0 ||
    strcmp(commandText, "off") == 0
  ) {
    return REMOTE_COMMAND_ALL_OFF;
  }

  if (
    strcmp(commandText, "stop_test") == 0 ||
    strcmp(commandText, "stop") == 0
  ) {
    return REMOTE_COMMAND_STOP_TEST;
  }

  if (strcmp(commandText, "ping") == 0) {
    return REMOTE_COMMAND_PING;
  }

  if (
    strcmp(commandText, "help") == 0 ||
    strcmp(commandText, "capabilities") == 0
  ) {
    return REMOTE_COMMAND_HELP;
  }

  return REMOTE_COMMAND_NONE;
}

const char* getRemoteCommandName(
  RemoteCommand command
) {
  switch (command) {
    case REMOTE_COMMAND_DEVICE_STATUS:
      return "device_status";

    case REMOTE_COMMAND_CHANNEL_SCAN:
      return "channel_scan";

    case REMOTE_COMMAND_APS:
      return "aps";

    case REMOTE_COMMAND_LOD_LSD:
      return "lod_lsd";

    case REMOTE_COMMAND_NEG_GCLK:
      return "neg_gclk";

    case REMOTE_COMMAND_CLEAR_ERRORS:
      return "clear_errors";

    case REMOTE_COMMAND_SET_GS:
      return "set_gs";

    case REMOTE_COMMAND_SET_DC:
      return "set_dc";

    case REMOTE_COMMAND_SET_ENABLE:
      return "set_enable";

    case REMOTE_COMMAND_RGB_CYCLE:
      return "rgb_cycle";

    case REMOTE_COMMAND_RGB_PHASE_ROTATION:
      return "rgb_phase_rotation";

    case REMOTE_COMMAND_LED_CHASE:
      return "led_chase";

    case REMOTE_COMMAND_BRIGHTNESS_SWEEP:
      return "brightness_sweep";

    case REMOTE_COMMAND_ALL_WHITE:
      return "all_white";

    case REMOTE_COMMAND_ALL_OFF:
      return "all_off";

    case REMOTE_COMMAND_STOP_TEST:
      return "stop_test";

    case REMOTE_COMMAND_PING:
      return "ping";

    case REMOTE_COMMAND_HELP:
      return "help";

    case REMOTE_COMMAND_NONE:
    default:
      return "unknown";
  }
}

void queueMqttCommandStatus(
  const char* command,
  const char* state,
  const char* detail
) {
  snprintf(
    mqttCommandStatusCommand,
    sizeof(mqttCommandStatusCommand),
    "%s",
    command != nullptr ? command : ""
  );

  snprintf(
    mqttCommandStatusState,
    sizeof(mqttCommandStatusState),
    "%s",
    state != nullptr ? state : ""
  );

  snprintf(
    mqttCommandStatusDetail,
    sizeof(mqttCommandStatusDetail),
    "%s",
    detail != nullptr ? detail : ""
  );

  mqttCommandStatusPublishPending = true;
}

void mqttMessageCallback(
  char* topic,
  uint8_t* payload,
  unsigned int length
) {
  if (strcmp(topic, MQTT_TOPIC_COMMAND) != 0) {
    return;
  }

  char commandText[40];

  const unsigned int copyLength =
    length < sizeof(commandText) - 1
      ? length
      : sizeof(commandText) - 1;

  memcpy(commandText, payload, copyLength);
  commandText[copyLength] = '\0';

  char* start = commandText;

  while (
    *start != '\0' &&
    isspace(static_cast<unsigned char>(*start))
  ) {
    start++;
  }

  char* end = start + strlen(start);

  while (
    end > start &&
    isspace(
      static_cast<unsigned char>(*(end - 1))
    )
  ) {
    end--;
  }

  *end = '\0';

  // Ignore the empty retained message used to clear stale commands.
  if (*start == '\0') {
    return;
  }

  for (
    char* character = start;
    *character != '\0';
    character++
  ) {
    *character = static_cast<char>(
      tolower(
        static_cast<unsigned char>(*character)
      )
    );
  }

  Serial.print("MQTT command received: ");
  Serial.println(start);

  if (pendingRemoteCommand != REMOTE_COMMAND_NONE) {
    queueMqttCommandStatus(
      "busy_request",
      "busy",
      "Another remote command is already queued."
    );
    return;
  }

  // Dynamic grayscale command, e.g. gs:R0:2048.
  // Callback validates/queues only; TLC write occurs from loop().
  if (strncmp(start, "gs:", 3) == 0) {
    char channelName[4] = "";
    unsigned int gsValue = 0;

    const int parsedFields = sscanf(
      start + 3,
      "%3[^:]:%u",
      channelName,
      &gsValue
    );

    const int8_t channelIndex =
      findChannelIndexByName(channelName);

    if (
      parsedFields != 2 ||
      channelIndex < 0 ||
      gsValue > 4095U
    ) {
      queueMqttCommandStatus(
        "set_gs",
        "rejected",
        "Use gs:R0:0 through gs:B7:4095."
      );
      return;
    }

    pendingRemoteChannel =
      static_cast<uint8_t>(channelIndex);

    pendingRemoteValue =
      static_cast<uint16_t>(gsValue);

    pendingRemoteCommand =
      REMOTE_COMMAND_SET_GS;

    snprintf(
      pendingRemoteCommandText,
      sizeof(pendingRemoteCommandText),
      "set_gs"
    );

    queueMqttCommandStatus(
      "set_gs",
      "queued",
      "GS update accepted for execution from loop()."
    );
    return;
  }


// Dynamic dot-correction command, e.g. dc:R0:64.
if (strncmp(start, "dc:", 3) == 0) {
  char channelName[4] = "";
  unsigned int dcValue = 0;

  const int parsedFields = sscanf(
    start + 3,
    "%3[^:]:%u",
    channelName,
    &dcValue
  );

  const int8_t channelIndex =
    findChannelIndexByName(channelName);

  if (
    parsedFields != 2 ||
    channelIndex < 0 ||
    dcValue > 127U
  ) {
    queueMqttCommandStatus(
      "set_dc",
      "rejected",
      "Use dc:R0:0 through dc:B7:127."
    );
    return;
  }

  pendingRemoteChannel =
    static_cast<uint8_t>(channelIndex);

  pendingRemoteValue =
    static_cast<uint16_t>(dcValue);

  pendingRemoteCommand =
    REMOTE_COMMAND_SET_DC;

  snprintf(
    pendingRemoteCommandText,
    sizeof(pendingRemoteCommandText),
    "set_dc"
  );

  queueMqttCommandStatus(
    "set_dc",
    "queued",
    "DC update accepted for execution from loop()."
  );

  return;
}
// Channel enable/disable:
// enable:R0:1 = enabled
// enable:R0:0 = disabled
if (strncmp(start, "enable:", 7) == 0) {
  char channelName[4] = "";
  unsigned int enableValue = 0;

  const int parsedFields = sscanf(
    start + 7,
    "%3[^:]:%u",
    channelName,
    &enableValue
  );

  const int8_t channelIndex =
    findChannelIndexByName(channelName);

  if (
    parsedFields != 2 ||
    channelIndex < 0 ||
    enableValue > 1U
  ) {
    queueMqttCommandStatus(
      "set_enable",
      "rejected",
      "Use enable:R0:0 or enable:R0:1."
    );
    return;
  }
  

  pendingRemoteChannel =
    static_cast<uint8_t>(channelIndex);

  pendingRemoteValue =
    static_cast<uint16_t>(enableValue);

  pendingRemoteCommand =
    REMOTE_COMMAND_SET_ENABLE;

  snprintf(
    pendingRemoteCommandText,
    sizeof(pendingRemoteCommandText),
    "set_enable"
  );

  queueMqttCommandStatus(
    "set_enable",
    "queued",
    "Channel enable update accepted."
  );

  return;
}
  const RemoteCommand parsedCommand =
    parseRemoteCommand(start);

  if (parsedCommand == REMOTE_COMMAND_NONE) {
    queueMqttCommandStatus(
      "unknown",
      "rejected",
      "Unknown command. Publish help for the command list."
    );
    return;
  }

  pendingRemoteCommand = parsedCommand;

  snprintf(
    pendingRemoteCommandText,
    sizeof(pendingRemoteCommandText),
    "%s",
    getRemoteCommandName(parsedCommand)
  );

  // Callback only parses and queues. No TLC work happens here.
  queueMqttCommandStatus(
    pendingRemoteCommandText,
    "queued",
    "Accepted for execution from loop()."
  );
}

void publishMqttCommandStatus() {
  if (!mqttClient.connected()) {
    return;
  }

  char payload[384];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"command\":\"%s\","
      "\"state\":\"%s\","
      "\"detail\":\"%s\","
      "\"err_active\":%s"
    "}",
    MQTT_DEVICE_ID,
    mqttCommandStatusCommand,
    mqttCommandStatusState,
    mqttCommandStatusDetail,
    isErrActive() ? "true" : "false"
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_COMMAND_STATUS,
      payload,
      false
    )
  ) {
    mqttCommandStatusPublishPending = false;
  }
}

void publishMqttCapabilities() {
  if (!mqttClient.connected()) {
    return;
  }

  char payload[1024];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"stage\":5,"
      "\"command_topic\":\"%s\","
      "\"commands\":["
        "\"device_status\","
        "\"channel_scan\","
        "\"aps\","
        "\"lod_lsd\","
        "\"neg_gclk\","
        "\"clear_errors\","
        "\"gs:<channel>:<0-4095>\","
        "\"dc:<channel>:<0-127>\","
        "\"enable:<channel>:<0|1>\","
        "\"rgb_cycle\","
        "\"rgb_phase_rotation\","
        "\"led_chase\","
        "\"brightness_sweep\","
        "\"all_white\","
        "\"all_off\","
        "\"stop_test\","
        "\"ping\","
        "\"help\""
      "],"
      "\"retain_commands\":false,"
      "\"callback_execution\":false,"
      "\"local_buttons_enabled\":true,"
      "\"automatic_tests_non_blocking\":true"
    "}",
    MQTT_DEVICE_ID,
    MQTT_TOPIC_COMMAND
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_CAPABILITIES,
      payload,
      true
    )
  ) {
    mqttCapabilitiesPublishPending = false;
  }
}

void queueMqttChannelControlPublish(
  uint8_t channelIndex
) {
  if (channelIndex >= CHANNEL_COUNT) {
    return;
  }

  mqttChannelControlIndex = channelIndex;
  mqttChannelControlPublishPending = true;
}

void publishMqttChannelControl() {
  if (
    !mqttClient.connected() ||
    !mqttChannelControlPublishPending
  ) {
    return;
  }

  const uint8_t channelIndex =
    mqttChannelControlIndex;

  char payload[320];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"channel\":\"%s\","
      "\"gs\":%u,"
      "\"dc\":%u,"
      "\"enabled\":%s"
    "}",
    MQTT_DEVICE_ID,
    CHANNEL_NAMES[channelIndex],
    static_cast<unsigned int>(gsValues[channelIndex]),
    static_cast<unsigned int>(dcValues[channelIndex]),
    channelEnabled[channelIndex] ? "true" : "false"
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_CHANNEL_CONTROL,
      payload,
      true
    )
  ) {
    mqttChannelControlPublishPending = false;
  }
}

void queueMqttAutomaticTestStatus(
  const char* testName,
  const char* state
) {
  snprintf(
    mqttAutomaticTestName,
    sizeof(mqttAutomaticTestName),
    "%s",
    testName != nullptr ? testName : "none"
  );

  snprintf(
    mqttAutomaticTestState,
    sizeof(mqttAutomaticTestState),
    "%s",
    state != nullptr ? state : "stopped"
  );

  mqttAutomaticTestPublishPending = true;
}

void publishMqttAutomaticTestStatus() {
  if (
    !mqttClient.connected() ||
    !mqttAutomaticTestPublishPending
  ) {
    return;
  }

  char payload[320];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"test\":\"%s\","
      "\"state\":\"%s\","
      "\"err_active\":%s"
    "}",
    MQTT_DEVICE_ID,
    mqttAutomaticTestName,
    mqttAutomaticTestState,
    isErrActive() ? "true" : "false"
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_AUTOMATIC_TEST,
      payload,
      true
    )
  ) {
    mqttAutomaticTestPublishPending = false;
  }
}


void runRemoteDiagnosticOption(
  uint8_t diagnosticOption
) {
  diagnosticsGridVisible = false;
  currentPage = PAGE_DIAGNOSTICS;
  selectedOption = diagnosticOption;

  drawDiagnosticsPage();
  runSelectedDiagnostic();
}

void servicePendingRemoteCommand() {
  if (pendingRemoteCommand == REMOTE_COMMAND_NONE) {
    return;
  }

  // Send queued acknowledgement before running the command.
  if (mqttCommandStatusPublishPending) {
    return;
  }

  const RemoteCommand command =
    pendingRemoteCommand;

  const uint8_t remoteChannel =
    pendingRemoteChannel;

  const uint16_t remoteValue =
    pendingRemoteValue;

  char executedCommand[40];

  snprintf(
    executedCommand,
    sizeof(executedCommand),
    "%s",
    pendingRemoteCommandText
  );

  pendingRemoteCommand = REMOTE_COMMAND_NONE;
  pendingRemoteCommandText[0] = '\0';

  Serial.println();
  Serial.print("Executing queued MQTT command: ");
  Serial.println(executedCommand);

  /*
   * Diagnostics and manual control own the GS output state.
   * Stop any running demonstration first so it cannot overwrite
   * their TLC frame on a later loop iteration.
   *
   * ping/help do not touch the LED state, and starting a different
   * automatic test simply replaces the current demonstration.
   */
  const bool automaticTestCommand =
    command == REMOTE_COMMAND_RGB_CYCLE ||
    command == REMOTE_COMMAND_RGB_PHASE_ROTATION ||
    command == REMOTE_COMMAND_LED_CHASE ||
    command == REMOTE_COMMAND_BRIGHTNESS_SWEEP ||
    command == REMOTE_COMMAND_ALL_WHITE ||
    command == REMOTE_COMMAND_ALL_OFF ||
    command == REMOTE_COMMAND_STOP_TEST;

  const bool stateNeutralCommand =
    command == REMOTE_COMMAND_PING ||
    command == REMOTE_COMMAND_HELP;

  if (
    activeAutomaticTest != AUTO_TEST_NONE &&
    !automaticTestCommand &&
    !stateNeutralCommand
  ) {
    stopAutomaticTest(true);
  }

  switch (command) {
    case REMOTE_COMMAND_DEVICE_STATUS:
      runRemoteDiagnosticOption(4);
      break;

    case REMOTE_COMMAND_CHANNEL_SCAN:
      runRemoteDiagnosticOption(3);
      break;

    case REMOTE_COMMAND_APS:
      runRemoteDiagnosticOption(2);
      break;

    case REMOTE_COMMAND_LOD_LSD:
      runRemoteDiagnosticOption(5);
      break;

    case REMOTE_COMMAND_NEG_GCLK:
      runRemoteDiagnosticOption(0);
      break;

    case REMOTE_COMMAND_CLEAR_ERRORS:
      runRemoteDiagnosticOption(1);
      break;

    case REMOTE_COMMAND_SET_GS:
      if (remoteChannel >= CHANNEL_COUNT) {
        queueMqttCommandStatus(
          "set_gs",
          "failed",
          "Channel index became invalid."
        );
        return;
      }

      gsValues[remoteChannel] =
        remoteValue > 0x0FFF
          ? 0x0FFF
          : remoteValue;

      applyGsValuesFast();

      if (
        currentPage == PAGE_MANUAL &&
        selectedChannel == remoteChannel
      ) {
        updateManualDynamicValues();
      }

      Serial.print("Remote GS update: ");
      Serial.print(CHANNEL_NAMES[remoteChannel]);
      Serial.print(" = ");
      Serial.println(gsValues[remoteChannel]);

      queueMqttChannelControlPublish(remoteChannel);
      break;

    case REMOTE_COMMAND_SET_DC:
      if (remoteChannel >= CHANNEL_COUNT) {
        queueMqttCommandStatus(
          "set_dc",
          "failed",
          "Channel index became invalid."
        );
        return;
      }

      dcValues[remoteChannel] =
        remoteValue > 127
          ? 127
          : static_cast<uint8_t>(remoteValue);

      applyFcValuesFast();

      if (
        currentPage == PAGE_MANUAL &&
        selectedChannel == remoteChannel
      ) {
        updateManualDynamicValues();
      }

      Serial.print("Remote DC update: ");
      Serial.print(CHANNEL_NAMES[remoteChannel]);
      Serial.print(" = ");
      Serial.println(dcValues[remoteChannel]);

      queueMqttChannelControlPublish(remoteChannel);
      break;

    case REMOTE_COMMAND_SET_ENABLE:
      if (remoteChannel >= CHANNEL_COUNT) {
        queueMqttCommandStatus(
          "set_enable",
          "failed",
          "Channel index became invalid."
        );
        return;
      }

      channelEnabled[remoteChannel] =
        (remoteValue != 0);

      applyGsValuesFast();

      if (
        currentPage == PAGE_MANUAL &&
        selectedChannel == remoteChannel
      ) {
        updateManualDynamicValues();
      }

      Serial.print("Remote channel ");
      Serial.print(CHANNEL_NAMES[remoteChannel]);
      Serial.print(": ");
      Serial.println(
        channelEnabled[remoteChannel]
          ? "ENABLED"
          : "DISABLED"
      );

      queueMqttChannelControlPublish(remoteChannel);
      break;

    case REMOTE_COMMAND_RGB_CYCLE:
      startAutomaticTest(AUTO_TEST_RGB_CYCLE);
      break;

    case REMOTE_COMMAND_RGB_PHASE_ROTATION:
      startAutomaticTest(AUTO_TEST_RGB_PHASE_ROTATION);
      break;

    case REMOTE_COMMAND_LED_CHASE:
      startAutomaticTest(AUTO_TEST_LED_CHASE);
      break;

    case REMOTE_COMMAND_BRIGHTNESS_SWEEP:
      startAutomaticTest(AUTO_TEST_BRIGHTNESS_SWEEP);
      break;

    case REMOTE_COMMAND_ALL_WHITE:
      startAutomaticTest(AUTO_TEST_ALL_WHITE);
      break;

    case REMOTE_COMMAND_ALL_OFF:
      startAutomaticTest(AUTO_TEST_ALL_OFF);
      break;

    case REMOTE_COMMAND_STOP_TEST:
      stopAutomaticTest(true);
      break;

    case REMOTE_COMMAND_PING:
      queueMqttErrStatusPublish();
      break;

    case REMOTE_COMMAND_HELP:
      mqttCapabilitiesPublishPending = true;
      break;

    case REMOTE_COMMAND_NONE:
    default:
      queueMqttCommandStatus(
        executedCommand,
        "failed",
        "Command state became invalid."
      );
      return;
  }

  queueMqttCommandStatus(
    executedCommand,
    "completed",
    "Command execution finished."
  );
}

bool publishMqttMessage(
  const char* topic,
  const char* payload,
  bool retained
) {
  if (!mqttClient.connected()) {
    return false;
  }

  const bool published =
    mqttClient.publish(topic, payload, retained);

  Serial.print("MQTT publish ");
  Serial.print(published ? "OK: " : "FAILED: ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(payload);

  return published;
}

void publishMqttHeartbeat() {
  char payload[192];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"uptime_ms\":%lu,"
      "\"rssi_dbm\":%ld,"
      "\"err_active\":%s"
    "}",
    MQTT_DEVICE_ID,
    static_cast<unsigned long>(millis()),
    static_cast<long>(WiFi.RSSI()),
    isErrActive() ? "true" : "false"
  );

  publishMqttMessage(
    MQTT_TOPIC_HEARTBEAT,
    payload,
    false
  );
}

void publishMqttErrStatus() {
  char payload[192];

  const bool errActive =
    isErrActive();

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"active\":%s,"
      "\"pin_level\":\"%s\""
    "}",
    MQTT_DEVICE_ID,
    errActive ? "true" : "false",
    errActive ? "LOW" : "HIGH"
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_ERR,
      payload,
      true
    )
  ) {
    mqttErrPublishPending = false;
  }
}

const char* getMqttLedStatusCode(
  LedDiagnosticStatus status
) {
  switch (status) {
    case LED_STATUS_OK:
      return "ok";

    case LED_STATUS_OPEN:
      return "open";

    case LED_STATUS_SHORT:
      return "short";

    case LED_STATUS_OUTPUT_SHORT_GND:
      return "out_gnd";

    case LED_STATUS_PWM_OR_UNKNOWN:
    default:
      return "unknown";
  }
}

void publishMqttChannelScan() {
  if (!mqttClient.connected()) {
    return;
  }

  String payload;
  payload.reserve(700);

  payload += "{\"device\":\"";
  payload += MQTT_DEVICE_ID;
  payload += "\",\"test\":\"channel_scan\"";

  payload += ",\"open\":";
  payload += scanOpenCount;

  payload += ",\"short\":";
  payload += scanShortCount;

  payload += ",\"out_gnd\":";
  payload += scanGroundCount;

  payload += ",\"unknown\":";
  payload += scanUnknownCount;

  payload += ",\"err_active\":";
  payload += isErrActive() ? "true" : "false";

  payload += ",\"channels\":{";

  bool firstChannel = true;

  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    if (!TLC_OUTPUT_CONNECTED[outputIndex]) {
      continue;
    }

    if (!firstChannel) {
      payload += ",";
    }

    payload += "\"";
    payload += TLC_OUTPUT_NAMES[outputIndex];
    payload += "\":\"";
    payload += getMqttLedStatusCode(
      allOutputStatus[outputIndex]
    );
    payload += "\"";

    firstChannel = false;
  }

  payload += "}}";

  if (
    publishMqttMessage(
      MQTT_TOPIC_CHANNEL_SCAN,
      payload.c_str(),
      true
    )
  ) {
    mqttChannelScanPublishPending = false;
  }
}

void publishMqttApsResult() {
  if (!mqttClient.connected()) {
    return;
  }

  String payload;
  payload.reserve(512);

  const char* result = "invalid";

  if (apsFlag == 0b011 && apsFaultCount == 0) {
    result = "pass";
  }
  else if (apsFlag == 0b110) {
    result = "fail";
  }

  payload += "{\"device\":\"";
  payload += MQTT_DEVICE_ID;
  payload += "\",\"test\":\"aps\",\"result\":\"";
  payload += result;
  payload += "\"";

  payload += ",\"flag\":\"";
  payload += static_cast<char>('0' + ((apsFlag >> 2) & 0x01));
  payload += static_cast<char>('0' + ((apsFlag >> 1) & 0x01));
  payload += static_cast<char>('0' + (apsFlag & 0x01));
  payload += "\"";

  payload += ",\"fault_count\":";
  payload += apsFaultCount;

  payload += ",\"err_active\":";
  payload += isErrActive() ? "true" : "false";

  payload += ",\"outputs\":[";

  bool firstFault = true;

  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    if (!apsOutputFault[outputIndex]) {
      continue;
    }

    if (!firstFault) {
      payload += ",";
    }

    payload += "\"";
    payload += TLC_OUTPUT_NAMES[outputIndex];
    payload += "\"";

    firstFault = false;
  }

  payload += "]}";

  if (
    publishMqttMessage(
      MQTT_TOPIC_APS,
      payload.c_str(),
      true
    )
  ) {
    mqttApsPublishPending = false;
  }
}

void publishMqttDeviceStatus() {
  if (!mqttClient.connected()) {
    return;
  }

  const char* result = "ok";

  if (deviceTefFlag) {
    result = "thermal_shutdown";
  }
  else if (devicePtwFlag) {
    result = "thermal_warning";
  }
  else if (deviceIsfFlag) {
    result = "iref_short";
  }
  else if (deviceIofFlag) {
    result = "iref_open";
  }
  else if (isErrActive()) {
    result = "err_active";
  }

  char payload[384];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"test\":\"device_status\","
      "\"result\":\"%s\","
      "\"tef\":%s,"
      "\"ptw\":%s,"
      "\"isf\":%s,"
      "\"iof\":%s,"
      "\"err_active\":%s"
    "}",
    MQTT_DEVICE_ID,
    result,
    deviceTefFlag ? "true" : "false",
    devicePtwFlag ? "true" : "false",
    deviceIsfFlag ? "true" : "false",
    deviceIofFlag ? "true" : "false",
    isErrActive() ? "true" : "false"
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_DEVICE_STATUS,
      payload,
      true
    )
  ) {
    mqttDeviceStatusPublishPending = false;
  }
}

void publishMqttLodLsdResult() {
  if (!mqttClient.connected()) {
    return;
  }

  const char* result = "invalid";

  if (lodLsdSelfTestFlag == 0b011) {
    result = "pass";
  }
  else if (lodLsdSelfTestFlag == 0b110) {
    result = "fail";
  }

  char payload[320];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"test\":\"lod_lsd_self_test\","
      "\"result\":\"%s\","
      "\"flag\":\"%u%u%u\","
      "\"err_active\":%s"
    "}",
    MQTT_DEVICE_ID,
    result,
    (lodLsdSelfTestFlag >> 2) & 0x01,
    (lodLsdSelfTestFlag >> 1) & 0x01,
    lodLsdSelfTestFlag & 0x01,
    isErrActive() ? "true" : "false"
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_LOD_LSD_SELF_TEST,
      payload,
      true
    )
  ) {
    mqttLodLsdPublishPending = false;
  }
}

void publishMqttNegGclkResult() {
  if (!mqttClient.connected()) {
    return;
  }

  char payload[512];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"test\":\"neg_gclk\","
      "\"result\":\"%s\","
      "\"connected_expected\":%u,"
      "\"connected_inversion_matches\":%u,"
      "\"connected_restore_matches\":%u,"
      "\"all_inversion_matches\":%u,"
      "\"all_restore_matches\":%u"
    "}",
    MQTT_DEVICE_ID,
    negGclkTestPassed ? "pass" : "fail",
    mqttNegConnectedExpected,
    negConnectedInversionMatchCount,
    mqttNegConnectedRestoreMatches,
    negInversionMatchCount,
    mqttNegAllRestoreMatches
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_NEG_GCLK,
      payload,
      true
    )
  ) {
    mqttNegGclkPublishPending = false;
  }
}

void publishMqttErrorClearResult() {
  if (!mqttClient.connected()) {
    return;
  }

  char payload[384];

  snprintf(
    payload,
    sizeof(payload),
    "{"
      "\"device\":\"%s\","
      "\"test\":\"error_clear\","
      "\"result\":\"%s\","
      "\"err_active\":%s,"
      "\"tef\":%s,"
      "\"ptw\":%s,"
      "\"isf\":%s,"
      "\"iof\":%s"
    "}",
    MQTT_DEVICE_ID,
    mqttErrorClearSucceeded ? "cleared" : "still_active",
    mqttErrorClearErrActive ? "true" : "false",
    mqttErrorClearTef ? "true" : "false",
    mqttErrorClearPtw ? "true" : "false",
    mqttErrorClearIsf ? "true" : "false",
    mqttErrorClearIof ? "true" : "false"
  );

  if (
    publishMqttMessage(
      MQTT_TOPIC_ERROR_CLEAR,
      payload,
      true
    )
  ) {
    mqttErrorClearPublishPending = false;
  }
}

void queueMqttErrStatusPublish() {
  /*
   * Only queue here. Actual network transmission happens from
   * loop(), after timing-sensitive TLC communication is done.
   */
  mqttErrPublishPending = true;
}

void queueMqttChannelScanPublish() {
  mqttChannelScanPublishPending = true;
}

void queueMqttApsPublish() {
  mqttApsPublishPending = true;
}

void queueMqttDeviceStatusPublish() {
  mqttDeviceStatusPublishPending = true;
}

void queueMqttLodLsdPublish() {
  mqttLodLsdPublishPending = true;
}

void queueMqttNegGclkPublish() {
  mqttNegGclkPublishPending = true;
}

void queueMqttErrorClearPublish(
  bool succeeded,
  bool errActive
) {
  mqttErrorClearSucceeded = succeeded;
  mqttErrorClearErrActive = errActive;

  mqttErrorClearTef = deviceTefFlag;
  mqttErrorClearPtw = devicePtwFlag;
  mqttErrorClearIsf = deviceIsfFlag;
  mqttErrorClearIof = deviceIofFlag;

  mqttErrorClearPublishPending = true;
}

void servicePendingDiagnosticPublishes() {
  /*
   * Send at most one diagnostic packet per loop iteration.
   * This prevents a burst of network work after a diagnostic.
   */
  if (mqttErrorClearPublishPending) {
    publishMqttErrorClearResult();
    return;
  }

  if (mqttDeviceStatusPublishPending) {
    publishMqttDeviceStatus();
    return;
  }

  if (mqttChannelScanPublishPending) {
    publishMqttChannelScan();
    return;
  }

  if (mqttApsPublishPending) {
    publishMqttApsResult();
    return;
  }

  if (mqttLodLsdPublishPending) {
    publishMqttLodLsdResult();
    return;
  }

  if (mqttNegGclkPublishPending) {
    publishMqttNegGclkResult();
  }
}

bool connectMqttBroker() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  configureMqttBrokerForCurrentWifi();

  Serial.print("Connecting to MQTT broker on port ");
  Serial.println(MQTT_PORT);

  const bool useAuthentication =
    MQTT_USERNAME[0] != '\0';

  bool connected = false;

  if (useAuthentication) {
    connected = mqttClient.connect(
      MQTT_DEVICE_ID,
      MQTT_USERNAME,
      MQTT_PASSWORD,
      MQTT_TOPIC_AVAILABILITY,
      0,
      true,
      "offline"
    );
  }
  else {
    connected = mqttClient.connect(
      MQTT_DEVICE_ID,
      MQTT_TOPIC_AVAILABILITY,
      0,
      true,
      "offline"
    );
  }

  if (!connected) {
    Serial.print("MQTT connection failed, state=");
    Serial.println(mqttClient.state());
    return false;
  }

  Serial.println("MQTT connected.");

  // Clear a retained command before subscribing to avoid replay on reconnect.
  if (
    !mqttClient.publish(
      MQTT_TOPIC_COMMAND,
      "",
      true
    )
  ) {
    Serial.println("MQTT stale-command clear FAILED.");
  }

  if (!mqttClient.subscribe(MQTT_TOPIC_COMMAND)) {
    Serial.println("MQTT command subscription FAILED.");
    mqttClient.disconnect();
    return false;
  }

  Serial.print("MQTT subscribed: ");
  Serial.println(MQTT_TOPIC_COMMAND);

  publishMqttMessage(
    MQTT_TOPIC_AVAILABILITY,
    "online",
    true
  );

  mqttErrPublishPending = true;
  mqttCapabilitiesPublishPending = true;

  queueMqttAutomaticTestStatus(
    getAutomaticTestName(activeAutomaticTest),
    activeAutomaticTest == AUTO_TEST_NONE
      ? "stopped"
      : "running"
  );

  queueMqttCommandStatus(
    "system",
    "ready",
    "Remote diagnostic command interface is ready."
  );

  previousMqttHeartbeatTime = 0;

  return true;
}

void serviceWifiAndMqtt() {
  const uint32_t now = millis();

  // Time spent on a Wi-Fi profile without a working MQTT connection.
  static uint32_t mqttDisconnectedSince = 0;

  // Network recovery must not block local controls.
  if (WiFi.status() != WL_CONNECTED) {

    // We are not connected to Wi-Fi, so reset the MQTT timer.
    mqttDisconnectedSince = 0;

    if (
      !wifiBeginIssued ||
      now - previousWifiAttemptTime >=
        WIFI_RETRY_INTERVAL_MS
    ) {
      Serial.println(
        "Wi-Fi disconnected; switching profile..."
      );

      // Alternate between the preferred and fallback Wi-Fi profiles.
      beginWifiProfile(!wifiTryingPrimaryProfile);
    }

    return;
  }

  if (!mqttClient.connected()) {

    if (mqttDisconnectedSince == 0) {
      mqttDisconnectedSince = now;
    }

    if (
      now - previousMqttAttemptTime >=
      MQTT_RETRY_INTERVAL_MS
    ) {
      previousMqttAttemptTime = now;
      connectMqttBroker();
    }

    // Return to the preferred hotspot if the fallback network has no broker.
    if (
      !wifiTryingPrimaryProfile &&
      now - mqttDisconnectedSince >=
        WIFI_PROFILE_RECOVERY_MS
    ) {
      Serial.println(
        "MQTT unreachable on current Wi-Fi; "
        "returning to demo hotspot..."
      );

      mqttDisconnectedSince = 0;

      mqttClient.disconnect();
      WiFi.disconnect();

      wifiBeginIssued = false;

      // Return to the preferred laptop hotspot.
      beginWifiProfile(true);

      return;
    }

    return;
  }

  mqttDisconnectedSince = 0;

  mqttClient.loop();

  if (mqttErrPublishPending) {
    publishMqttErrStatus();
  }

  if (mqttCapabilitiesPublishPending) {
    publishMqttCapabilities();
  }

  if (mqttCommandStatusPublishPending) {
    publishMqttCommandStatus();
  }

  if (mqttChannelControlPublishPending) {
    publishMqttChannelControl();
  }

  if (mqttAutomaticTestPublishPending) {
    publishMqttAutomaticTestStatus();
  }

  servicePendingDiagnosticPublishes();

  if (
    now - previousMqttHeartbeatTime >=
    MQTT_HEARTBEAT_INTERVAL_MS
  ) {
    previousMqttHeartbeatTime = now;
    publishMqttHeartbeat();
  }
}
