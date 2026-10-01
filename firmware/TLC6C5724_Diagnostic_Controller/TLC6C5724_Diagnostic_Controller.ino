#include <Arduino.h>
#include <string.h>
#include <ctype.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>

#include "ProjectConfig.h"



WiFiClient mqttNetworkClient;
PubSubClient mqttClient(mqttNetworkClient);
uint32_t previousWifiAttemptTime = 0;
uint32_t previousMqttAttemptTime = 0;
uint32_t previousMqttHeartbeatTime = 0;

bool wifiBeginIssued = false;
bool wifiTryingPrimaryProfile = true;
bool mqttErrPublishPending = true;

bool mqttChannelScanPublishPending = false;
bool mqttApsPublishPending = false;
bool mqttDeviceStatusPublishPending = false;
bool mqttLodLsdPublishPending = false;
bool mqttNegGclkPublishPending = false;
bool mqttErrorClearPublishPending = false;
bool mqttChannelControlPublishPending = false;
bool mqttAutomaticTestPublishPending = false;

uint8_t mqttChannelControlIndex = 0;

char mqttAutomaticTestName[32] = "none";
char mqttAutomaticTestState[16] = "stopped";

// Last Error Clear result used for MQTT status.
bool mqttErrorClearSucceeded = false;
bool mqttErrorClearErrActive = false;
bool mqttErrorClearTef = false;
bool mqttErrorClearPtw = false;
bool mqttErrorClearIsf = false;
bool mqttErrorClearIof = false;

// NEG/GCLK summary retained for MQTT publishing.
uint16_t mqttNegConnectedExpected = 0;
uint16_t mqttNegConnectedRestoreMatches = 0;
uint16_t mqttNegAllRestoreMatches = 0;

// MQTT remote-command state

enum RemoteCommand : uint8_t {
  REMOTE_COMMAND_NONE = 0,
  REMOTE_COMMAND_DEVICE_STATUS,
  REMOTE_COMMAND_CHANNEL_SCAN,
  REMOTE_COMMAND_APS,
  REMOTE_COMMAND_LOD_LSD,
  REMOTE_COMMAND_NEG_GCLK,
  REMOTE_COMMAND_CLEAR_ERRORS,
  REMOTE_COMMAND_SET_GS,
  REMOTE_COMMAND_SET_DC,
  REMOTE_COMMAND_SET_ENABLE,
  REMOTE_COMMAND_RGB_CYCLE,
  REMOTE_COMMAND_RGB_PHASE_ROTATION,
  REMOTE_COMMAND_LED_CHASE,
  REMOTE_COMMAND_BRIGHTNESS_SWEEP,
  REMOTE_COMMAND_ALL_WHITE,
  REMOTE_COMMAND_ALL_OFF,
  REMOTE_COMMAND_STOP_TEST,
  REMOTE_COMMAND_PING,
  REMOTE_COMMAND_HELP
};

RemoteCommand pendingRemoteCommand =
  REMOTE_COMMAND_NONE;

char pendingRemoteCommandText[40] = "";


uint8_t pendingRemoteChannel = 0;
uint16_t pendingRemoteValue = 0;

bool mqttCommandStatusPublishPending = false;
bool mqttCapabilitiesPublishPending = true;

char mqttCommandStatusCommand[40] = "";
char mqttCommandStatusState[24] = "";
char mqttCommandStatusDetail[128] = "";

// ESP32 -> TLC6C5724 connections
#define PIN_MOSI   13   // TLC SDI, pin 1
#define PIN_MISO   27   // TLC SDO, pin 19
#define PIN_SCK    14   // TLC SCK, pin 2
#define PIN_LATCH  15   // TLC LATCH, pin 3
#define PIN_BLANK  4    // TLC BLANK, pin 36
#define PIN_GCLK   25   // TLC GCLK, pins 4/5/6
#define PIN_ERR    32   // TLC ERR, pin 20 (active-low, open-drain)

// Three physical TFT-menu buttons
#define PIN_BUTTON_PAGE    19
#define PIN_BUTTON_OPTION  21
#define PIN_BUTTON_SELECT  26

// TLC grayscale clock
constexpr uint32_t GCLK_FREQUENCY_HZ = 1000000;
constexpr uint8_t  GCLK_RESOLUTION_BITS = 5;
constexpr uint32_t GCLK_DUTY = 16;  // Approx. 50% of 0-31

// TFT pins are configured in TFT_eSPI/User_Setup.h
TFT_eSPI tft = TFT_eSPI();

// TLC common 288-bit shift-register frame
uint8_t frame[36];

// Readback buffer for SID and status data.
uint8_t sidFrame[36];

// Size of one 288-bit TLC frame.
constexpr size_t TLC_FRAME_BYTES = sizeof(sidFrame);

// Manual and remote channel state
constexpr uint8_t CHANNEL_COUNT = 24;

const char* const CHANNEL_NAMES[CHANNEL_COUNT] = {
  "R0", "G0", "B0", "R1", "G1", "B1",
  "R2", "G2", "B2", "R3", "G3", "B3",
  "R4", "G4", "B4", "R5", "G5", "B5",
  "R6", "G6", "B6", "R7", "G7", "B7"
};

// Startup pattern used for bench testing; other channels start at zero.
uint16_t gsValues[CHANNEL_COUNT] = {
  0xFFF, 0x000, 0x000,
  0x000, 0xFFF, 0x000,
  0x000, 0x000, 0xFFF,
  0x000, 0x000, 0x000,
  0x000, 0x000, 0x000,
  0x000, 0x000, 0x000,
  0x000, 0x000, 0x000,
  0x000, 0x000, 0x000
};

bool channelEnabled[CHANNEL_COUNT] = {
  true, true, true,
  true, true, true,
  true, true, true,
  false, false, false,
  false, false, false,
  false, false, false,
  false, false, false,
  false, false, false
};

uint8_t selectedChannel = 0;

// Preset grayscale levels used by the local controls.
constexpr uint16_t GS_LEVELS[] = {
  0x000,
  0x080,
  0x100,
  0x200,
  0x400,
  0x600,
  0x800,
  0xA00,
  0xC00,
  0xE00,
  0xFFF
};

constexpr size_t GS_LEVEL_COUNT =
  sizeof(GS_LEVELS) / sizeof(GS_LEVELS[0]);

// Per-channel 7-bit dot-correction values.
// Range: 0 ... 127
uint8_t dcValues[CHANNEL_COUNT] = {
  127, 127, 127, 127, 127, 127,
  127, 127, 127, 127, 127, 127,
  127, 127, 127, 127, 127, 127,
  127, 127, 127, 127, 127, 127
};

// Preset dot-correction levels.
constexpr uint8_t DC_LEVELS[] = {
  0,
  16,
  32,
  48,
  64,
  80,
  96,
  112,
  127
};

constexpr size_t DC_LEVEL_COUNT =
  sizeof(DC_LEVELS) / sizeof(DC_LEVELS[0]);

// Group brightness-control values.
// Index order: 0 = Red, 1 = Green, 2 = Blue
uint8_t bcValues[3] = {
  255,
  255,
  255
};

const char* const BC_GROUP_NAMES[3] = {
  "R",
  "G",
  "B"
};

// Preset group-brightness levels.
constexpr uint8_t BC_LEVELS[] = {
  0,
  32,
  64,
  96,
  128,
  160,
  192,
  224,
  255
};

constexpr size_t BC_LEVEL_COUNT =
  sizeof(BC_LEVELS) / sizeof(BC_LEVELS[0]);

// Temporary current profile used only for LOD/LSD diagnostics.
constexpr uint8_t DIAGNOSTIC_BC = 16;

// Three-button input
enum UiKey : uint8_t {
  KEY_NONE = 0,
  KEY_PAGE,
  KEY_OPTION,
  KEY_SELECT
};

struct ButtonState {
  uint8_t pin;
  UiKey key;
  uint8_t lastRawState;
  uint8_t stableState;
  uint32_t lastRawChangeTime;
};

constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;

ButtonState buttons[] = {
  {PIN_BUTTON_PAGE,   KEY_PAGE,   HIGH, HIGH, 0},
  {PIN_BUTTON_OPTION, KEY_OPTION, HIGH, HIGH, 0},
  {PIN_BUTTON_SELECT, KEY_SELECT, HIGH, HIGH, 0}
};

constexpr size_t BUTTON_COUNT =
  sizeof(buttons) / sizeof(buttons[0]);

// TFT pages: Overview -> Manual -> Diagnostics -> Automatic Tests
enum UiPage : uint8_t {
  PAGE_OVERVIEW = 0,
  PAGE_MANUAL,
  PAGE_DIAGNOSTICS,
  PAGE_TESTS,
  PAGE_COUNT
};

UiPage currentPage = PAGE_OVERVIEW;
uint8_t selectedOption = 0;

constexpr uint8_t MANUAL_OPTION_COUNT = 5;
constexpr uint8_t TEST_OPTION_COUNT = 6;
constexpr uint8_t DIAGNOSTIC_OPTION_COUNT = 6;

// ERR is active-low:
// HIGH = no active fault
// LOW  = TLC is reporting a fault
bool lastErrActive = false;
bool errStatusInitialized = false;

constexpr uint32_t ERR_POLL_INTERVAL_MS = 100;
uint32_t previousErrPollTime = 0;

// Result decoded from LOD1, LOD2, LSD1 and LSD2
// for one selected TLC output.
enum LedDiagnosticStatus : uint8_t {
  LED_STATUS_OK = 0,
  LED_STATUS_OPEN,
  LED_STATUS_SHORT,
  LED_STATUS_OUTPUT_SHORT_GND,
  LED_STATUS_PWM_OR_UNKNOWN
};

constexpr uint8_t TLC_OUTPUT_COUNT = 24;

const char* const TLC_OUTPUT_NAMES[TLC_OUTPUT_COUNT] = {
  "R0", "G0", "B0", "R1", "G1", "B1",
  "R2", "G2", "B2", "R3", "G3", "B3",
  "R4", "G4", "B4", "R5", "G5", "B5",
  "R6", "G6", "B6", "R7", "G7", "B7"
};

// Channels populated on the current validation fixture.
const bool TLC_OUTPUT_CONNECTED[TLC_OUTPUT_COUNT] = {
  true, true, true, true, true, true,
  true, true, true, false, false, false,
  false, false, false, false, false, false,
  false, false, false, false, false, false
};

LedDiagnosticStatus allOutputStatus[TLC_OUTPUT_COUNT];

bool diagnosticsGridVisible = false;

uint8_t scanOpenCount = 0;
uint8_t scanShortCount = 0;
uint8_t scanGroundCount = 0;
uint8_t scanUnknownCount = 0;

// Adjacent-pin-short (APS) diagnostic result.
//   0b011 = pass
//   0b110 = adjacent-pin short detected
uint8_t apsFlag = 0;
bool apsOutputFault[TLC_OUTPUT_COUNT] = {false};
uint8_t apsFaultCount = 0;

// Global device-diagnostic flags read from SID.
bool deviceTefFlag = false;   // Thermal shutdown
bool devicePtwFlag = false;   // Pre-thermal warning
bool deviceIsfFlag = false;   // IREF short
bool deviceIofFlag = false;   // IREF open

// LOD/LSD detector-circuit self-test result from SID bits 208..206.
//   0b011 = self-test pass
//   0b110 = self-test fail
uint8_t lodLsdSelfTestFlag = 0;

// NEG/GCLK register-integrity diagnostic.
// The four LOD/LSD fields contain 96 relevant status bits in total.
constexpr uint16_t NEG_REGISTER_BIT_COUNT = 96;

constexpr uint16_t NEG_GCLK_WAIT_MS = 25;

bool negGclkTestPassed = false;
uint16_t negInversionMatchCount = 0;
uint16_t negConnectedInversionMatchCount = 0;


// Non-blocking automatic demonstration tests
enum AutomaticTest : uint8_t {
  AUTO_TEST_NONE = 0,
  AUTO_TEST_RGB_CYCLE,
  AUTO_TEST_RGB_PHASE_ROTATION,
  AUTO_TEST_LED_CHASE,
  AUTO_TEST_BRIGHTNESS_SWEEP,
  AUTO_TEST_ALL_WHITE,
  AUTO_TEST_ALL_OFF
};

AutomaticTest activeAutomaticTest = AUTO_TEST_NONE;

uint32_t previousAutomaticTestStepTime = 0;
uint8_t automaticTestStep = 0;
int8_t automaticBrightnessDirection = 1;

// Demo level for static RGB, chase, and white tests.
constexpr uint16_t DEMO_TEST_GS = 0x0800;

constexpr uint32_t RGB_CYCLE_STEP_MS = 650;
constexpr uint32_t RGB_PHASE_ROTATION_STEP_MS = 450;
constexpr uint32_t LED_CHASE_STEP_MS = 250;
constexpr uint32_t BRIGHTNESS_SWEEP_STEP_MS = 12;

// Function declarations

enum LedDiagnosticStatus : uint8_t;

// Wi-Fi and MQTT
void beginWifiConnection();
void beginWifiProfile(bool primaryProfile);
void configureMqttClient();
void configureMqttBrokerForCurrentWifi();
bool connectMqttBroker();
void serviceWifiAndMqtt();

void mqttMessageCallback(
  char* topic,
  uint8_t* payload,
  unsigned int length
);

RemoteCommand parseRemoteCommand(
  const char* commandText
);

const char* getRemoteCommandName(
  RemoteCommand command
);

void queueMqttCommandStatus(
  const char* command,
  const char* state,
  const char* detail
);

void publishMqttCommandStatus();
void publishMqttCapabilities();
void publishMqttChannelControl();
void publishMqttAutomaticTestStatus();

void queueMqttAutomaticTestStatus(
  const char* testName,
  const char* state
);

int8_t findChannelIndexByName(
  const char* channelName
);

void queueMqttChannelControlPublish(
  uint8_t channelIndex
);

void servicePendingRemoteCommand();

void runRemoteDiagnosticOption(
  uint8_t diagnosticOption
);

bool publishMqttMessage(
  const char* topic,
  const char* payload,
  bool retained
);

void publishMqttHeartbeat();
void publishMqttErrStatus();

const char* getMqttLedStatusCode(
  LedDiagnosticStatus status
);

void publishMqttChannelScan();
void publishMqttApsResult();
void publishMqttDeviceStatus();
void publishMqttLodLsdResult();
void publishMqttNegGclkResult();
void publishMqttErrorClearResult();

void queueMqttErrStatusPublish();
void queueMqttChannelScanPublish();
void queueMqttApsPublish();
void queueMqttDeviceStatusPublish();
void queueMqttLodLsdPublish();
void queueMqttNegGclkPublish();
void queueMqttErrorClearPublish(
  bool succeeded,
  bool errActive
);

void servicePendingDiagnosticPublishes();

// TLC frame helpers
void clearFrame();
void setBitTLC(uint16_t bit, bool value);
bool getBitTLC(uint16_t bit);
void setFieldTLC(uint16_t msb, uint16_t lsb, uint32_t value);

void clearSidFrame();
void setBufferBit(
  uint8_t* buffer,
  uint16_t bit,
  bool value
);
bool getBufferBit(
  const uint8_t* buffer,
  uint16_t bit
);

// TLC communication
void pulseSCK();
void pulseSCKFast();

void sendFCFrame();
void sendGSFrame();
void sendGSFrameFast();
void sendFCFrameFast();
void applyFcValuesFast();

void applyGsValuesFast();
void sendSpecialCommand(uint16_t command);
void readSidIntoBuffer();

// TLC configuration
void buildFCThreeRGB();
void buildGSThreeRGB();
void buildDiagnosticGsSingleOutput(uint8_t outputIndex);
void applyGsValues();
void applyFcValues();

void beginDiagnosticCurrentProfile(
  uint8_t savedBc[3]
);

void restoreDiagnosticCurrentProfile(
  const uint8_t savedBc[3]
);

const char* getSelectedChannelName();
uint8_t getSelectedBcGroup();
const char* getSelectedBcGroupName();
size_t findClosestGsLevel(uint16_t value);
size_t findClosestDcLevel(uint8_t value);
size_t findClosestBcLevel(uint8_t value);
void cycleSelectedManualValue();
void updateManualDynamicValues();
void updateManualValueField(
  int16_t y,
  uint8_t optionIndex,
  const char* value
);

// Physical buttons
void initializeButtons();
UiKey readUiButtons();
const char* getUiKeyName(UiKey key);

// UI helpers
uint8_t getOptionCount(UiPage page);
const char* getPageTitle(UiPage page);
void drawPageHeader();
void drawPageFooter();
void drawMenuRow(
  int16_t y,
  uint8_t optionIndex,
  const char* label,
  const char* value
);

// UI pages
void drawOverviewPage();
void drawManualPage();
void drawTestsPage();
void drawDiagnosticsPage();
void drawCurrentPage();

const char* getAutomaticTestName(
  AutomaticTest test
);

const char* getAutomaticTestLabel(
  AutomaticTest test
);

void showAutomaticTestMessage(
  const char* message,
  uint16_t color
);

void buildAutomaticRgbFrame(
  uint16_t red,
  uint16_t green,
  uint16_t blue
);

void buildAutomaticWhiteFrame(
  uint16_t level
);

void buildAutomaticPhaseRotationFrame(
  uint8_t phase,
  uint16_t level
);

void buildAutomaticChaseFrame(
  uint8_t ledIndex,
  uint8_t colourIndex,
  uint16_t level
);

void applyAutomaticTestFrame();
void renderAutomaticTestStep();

void startAutomaticTest(
  AutomaticTest test
);

void stopAutomaticTest(
  bool restoreManualState = true
);

void serviceAutomaticTest();
void runSelectedAutomaticTest();

void drawManualOption(uint8_t optionIndex);
void drawTestsOption(uint8_t optionIndex);
void drawDiagnosticsOption(uint8_t optionIndex);
void redrawCurrentPageOption(uint8_t optionIndex);

void drawDiagnosticsGrid();
void drawDiagnosticGridCell(
  uint8_t outputIndex,
  int16_t x,
  int16_t y
);

// Diagnostics
bool isErrActive();
void updateDiagnosticsErrStatus(bool forceRedraw = false);
void showDiagnosticsMessage(const char* message, uint16_t color);

void getOutputDiagnosticBits(
  uint8_t outputIndex,
  bool& lod1,
  bool& lod2,
  bool& lsd1,
  bool& lsd2
);

void getSelectedChannelDiagnosticBits(
  bool& lod1,
  bool& lod2,
  bool& lsd1,
  bool& lsd2
);

LedDiagnosticStatus classifyOutputSid(
  uint8_t outputIndex
);

LedDiagnosticStatus classifySelectedChannelSid();
const char* getLedDiagnosticStatusName(
  LedDiagnosticStatus status
);

LedDiagnosticStatus captureSelectedChannelSid();

void captureAllOutputSid();
void printAllOutputSid();

uint16_t getApsSidBit(uint8_t outputIndex);
void decodeApsResult();
void printApsResult();
void executeApsSpecialCommand(uint16_t command);
void runAdjacentPinTest();

void decodeDeviceStatusFlags();
void printDeviceStatusFlags();
void runDeviceStatusTest();

void decodeLodLsdSelfTestFlag();
void printLodLsdSelfTestResult();
void runLodLsdSelfTest();

bool isNegateControlledSidBit(uint16_t bit);
int8_t getNegateOutputIndexForSidBit(uint16_t bit);
const char* getNegateRegisterName(uint16_t bit);
bool isConnectedNegateSidBit(uint16_t bit);
uint16_t getConnectedNegateRegisterBitCount();

void buildNegGclkDiagnosticGsAllOutputs();

uint16_t countNegateInversionMatches(
  const uint8_t* beforeFrame,
  const uint8_t* afterFrame
);
uint16_t countConnectedNegateInversionMatches(
  const uint8_t* beforeFrame,
  const uint8_t* afterFrame
);
uint16_t countNegateEqualityMatches(
  const uint8_t* firstFrame,
  const uint8_t* secondFrame
);
uint16_t countConnectedNegateEqualityMatches(
  const uint8_t* firstFrame,
  const uint8_t* secondFrame
);

void printNegateMismatchedBits(
  const uint8_t* beforeFrame,
  const uint8_t* afterFrame
);
void printIgnoredNcNegateMismatches(
  const uint8_t* beforeFrame,
  const uint8_t* afterFrame
);

void runNegGclkIntegrityTest();

uint8_t getSidFieldByte(uint16_t msb);
void printByteBinary(uint8_t value);
void printSidRawFrame();
void printSidGroupFields();
void printSidBitWindow(
  const char* label,
  uint16_t centerBit
);
void runSidRawDebugForOutput(uint8_t outputIndex);
void runSidRawDebug();

void printSelectedChannelSid(
  LedDiagnosticStatus status
);

void runSelectedDiagnostic();

// UI navigation
void changePage(int8_t direction);
void changeOption(int8_t direction);
void activateSelectedOption();

void setup() {
  Serial.begin(115200);
  delay(800);

  // TLC pins
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_SCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_BLANK, OUTPUT);
  pinMode(PIN_GCLK, OUTPUT);
  pinMode(PIN_ERR, INPUT);  // External 9-10 kOhm pull-up to 3.3 V

  // Local controls
  initializeButtons();

  // Safe startup levels
  digitalWrite(PIN_MOSI, LOW);
  digitalWrite(PIN_SCK, LOW);
  digitalWrite(PIN_LATCH, HIGH);
  digitalWrite(PIN_BLANK, LOW);
  digitalWrite(PIN_GCLK, LOW);

  Serial.println();
  Serial.println("Starting TLC6C5724 + TFT menu system");

  // Display
  tft.init();
  tft.setRotation(1);  // 160 x 128 landscape
  tft.setTextWrap(false);
  drawCurrentPage();

  // Reset and initialize the TLC driver.
  executeApsSpecialCommand(0xA5C);  // Global reset
  delay(20);

  executeApsSpecialCommand(0xA53);  // Clear errors
  delay(20);

  buildFCThreeRGB();
  sendFCFrame();
  delay(20);

  buildGSThreeRGB();
  sendGSFrame();
  delay(20);

  // Start the TLC grayscale clock.
  if (!ledcAttach(
        PIN_GCLK,
        GCLK_FREQUENCY_HZ,
        GCLK_RESOLUTION_BITS)) {

    Serial.println("ERROR: GCLK hardware PWM setup failed.");
    digitalWrite(PIN_BLANK, LOW);

    while (true) {
      delay(1000);
    }
  }

  ledcWrite(PIN_GCLK, GCLK_DUTY);
  delayMicroseconds(100);

  // Enable outputs after GCLK is stable.
  digitalWrite(PIN_BLANK, HIGH);

  lastErrActive = isErrActive();
  errStatusInitialized = true;
  previousErrPollTime = millis();

  Serial.print("Initial ERR pin state: ");
  Serial.println(
    lastErrActive ? "LOW - ACTIVE" : "HIGH - OK"
  );

  Serial.println("Display and TLC driver ready.");
  Serial.println("Local diagnostics and LED controls ready.");

  configureMqttClient();
  beginWifiConnection();

  Serial.println("MQTT remote control enabled.");
  Serial.println("Buttons: PAGE, OPTION, SELECT.");
}


void loop() {
  const UiKey key = readUiButtons();

  if (key != KEY_NONE) {
    Serial.print("Detected key: ");
    Serial.println(getUiKeyName(key));
  }

  switch (key) {
    case KEY_PAGE:
      changePage(1);
      break;

    case KEY_OPTION:
      changeOption(1);
      break;

    case KEY_SELECT:
      activateSelectedOption();
      break;

    case KEY_NONE:
    default:
      break;
  }

  const uint32_t now = millis();

  if (now - previousErrPollTime >= ERR_POLL_INTERVAL_MS) {
    previousErrPollTime = now;
    updateDiagnosticsErrStatus(false);
  }

  // Give local button input priority over queued remote commands.
  if (key == KEY_NONE) {
    servicePendingRemoteCommand();
  }

  // Automatic tests remain non-blocking.
  serviceAutomaticTest();

  serviceWifiAndMqtt();

  delay(5);
}
