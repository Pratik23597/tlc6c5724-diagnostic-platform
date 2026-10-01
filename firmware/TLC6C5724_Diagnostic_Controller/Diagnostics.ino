uint16_t countConnectedNegateInversionMatches(
  const uint8_t* beforeFrame,
  const uint8_t* afterFrame
) {
  uint16_t matchCount = 0;

  for (uint16_t bit = 144; bit <= 287; bit++) {
    if (!isConnectedNegateSidBit(bit)) {
      continue;
    }

    const bool beforeValue =
      getBufferBit(beforeFrame, bit);

    const bool afterValue =
      getBufferBit(afterFrame, bit);

    if (afterValue == !beforeValue) {
      matchCount++;
    }
  }

  return matchCount;
}

uint16_t countConnectedNegateEqualityMatches(
  const uint8_t* firstFrame,
  const uint8_t* secondFrame
) {
  uint16_t matchCount = 0;

  for (uint16_t bit = 144; bit <= 287; bit++) {
    if (!isConnectedNegateSidBit(bit)) {
      continue;
    }

    if (
      getBufferBit(firstFrame, bit) ==
      getBufferBit(secondFrame, bit)
    ) {
      matchCount++;
    }
  }

  return matchCount;
}


void drawDiagnosticsGrid() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(3, 3);
  tft.print("ALL CHANNELS");

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(86, 3);
  tft.print("O:");
  tft.print(scanOpenCount);

  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setCursor(108, 3);
  tft.print("S:");
  tft.print(scanShortCount);

  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.setCursor(132, 3);
  tft.print("G:");
  tft.print(scanGroundCount);

  constexpr int16_t GRID_X = 2;
  constexpr int16_t GRID_Y = 16;
  constexpr int16_t CELL_STEP_X = 26;
  constexpr int16_t CELL_STEP_Y = 22;

  for (uint8_t row = 0; row < 4; row++) {
    for (uint8_t column = 0; column < 6; column++) {
      const uint8_t outputIndex =
        row * 6 + column;

      drawDiagnosticGridCell(
        outputIndex,
        GRID_X + column * CELL_STEP_X,
        GRID_Y + row * CELL_STEP_Y
      );
    }
  }

  tft.fillRect(0, 106, 160, 22, TFT_BLACK);
  tft.drawFastHLine(2, 106, 156, TFT_DARKGREY);

  tft.setTextSize(1);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(3, 112);
  tft.print("OK");

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(24, 112);
  tft.print("OPEN");

  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.setCursor(59, 112);
  tft.print("SHORT");

  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.setCursor(101, 112);
  tft.print("GND");

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(132, 112);
  tft.print("NC");
}

void drawDiagnosticsPage() {
  if (diagnosticsGridVisible) {
    drawDiagnosticsGrid();
    return;
  }

  drawPageHeader();

  updateDiagnosticsErrStatus(true);

  drawMenuRow(32, 0, "NEG/GCLK test", nullptr);
  drawMenuRow(45, 1, "Clear errors", nullptr);
  drawMenuRow(58, 2, "Adjacent pin test", nullptr);
  drawMenuRow(71, 3, "Scan all channels", nullptr);
  drawMenuRow(84, 4, "Device status", nullptr);
  drawMenuRow(97, 5, "LOD/LSD self-test", nullptr);

  drawPageFooter();
}

bool isErrActive() {
  return digitalRead(PIN_ERR) == LOW;
}

void updateDiagnosticsErrStatus(bool forceRedraw) {
  const bool errActive = isErrActive();

  if (
    !forceRedraw &&
    errStatusInitialized &&
    errActive == lastErrActive
  ) {
    return;
  }

  lastErrActive = errActive;
  errStatusInitialized = true;

  queueMqttErrStatusPublish();

  if (
    currentPage != PAGE_DIAGNOSTICS ||
    diagnosticsGridVisible
  ) {
    return;
  }

  // Update only the ERR status strip, not the complete page.
  tft.fillRect(4, 19, 152, 12, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(6, 21);

  if (errActive) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("ERR: ACTIVE (LOW)");
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("ERR: OK (HIGH)");
  }
}

void showDiagnosticsMessage(
  const char* message,
  uint16_t color
) {
  if (currentPage != PAGE_DIAGNOSTICS) {
    return;
  }

  tft.fillRect(0, 96, 160, 12, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(color, TFT_BLACK);
  tft.setCursor(5, 99);
  tft.print(message);
}

void getOutputDiagnosticBits(
  uint8_t outputIndex,
  bool& lod1,
  bool& lod2,
  bool& lsd1,
  bool& lsd2
) {
  if (outputIndex >= TLC_OUTPUT_COUNT) {
    lod1 = false;
    lod2 = false;
    lsd1 = false;
    lsd2 = false;
    return;
  }

  const uint8_t outputNumber =
    outputIndex / 3;

  const uint8_t colorGroup =
    outputIndex % 3;

  // Base bits for OUTR0, OUTG0 and OUTB0.
  static const uint16_t LOD2_BASE[3] = {
    264,
    272,
    280
  };

  static const uint16_t LOD1_BASE[3] = {
    240,
    248,
    256
  };

  static const uint16_t LSD2_BASE[3] = {
    168,
    176,
    184
  };

  static const uint16_t LSD1_BASE[3] = {
    144,
    152,
    160
  };

  lod2 = getBufferBit(
    sidFrame,
    LOD2_BASE[colorGroup] + outputNumber
  );

  lod1 = getBufferBit(
    sidFrame,
    LOD1_BASE[colorGroup] + outputNumber
  );

  lsd2 = getBufferBit(
    sidFrame,
    LSD2_BASE[colorGroup] + outputNumber
  );

  lsd1 = getBufferBit(
    sidFrame,
    LSD1_BASE[colorGroup] + outputNumber
  );
}

void getSelectedChannelDiagnosticBits(
  bool& lod1,
  bool& lod2,
  bool& lsd1,
  bool& lsd2
) {
  getOutputDiagnosticBits(
    selectedChannel,
    lod1,
    lod2,
    lsd1,
    lsd2
  );
}

LedDiagnosticStatus classifyOutputSid(
  uint8_t outputIndex
) {
  bool lod1 = false;
  bool lod2 = false;
  bool lsd1 = false;
  bool lsd2 = false;

  getOutputDiagnosticBits(
    outputIndex,
    lod1,
    lod2,
    lsd1,
    lsd2
  );

  if (!lod1 && !lod2 && !lsd1 && lsd2) {
    return LED_STATUS_OK;
  }

  if (lod1 && !lod2 && !lsd1 && lsd2) {
    return LED_STATUS_OPEN;
  }

  if (!lod1 && !lod2 && lsd1 && lsd2) {
    return LED_STATUS_SHORT;
  }

  if (lod1 && lod2 && !lsd1 && !lsd2) {
    return LED_STATUS_OUTPUT_SHORT_GND;
  }

  return LED_STATUS_PWM_OR_UNKNOWN;
}

LedDiagnosticStatus classifySelectedChannelSid() {
  return classifyOutputSid(selectedChannel);
}

const char* getLedDiagnosticStatusName(
  LedDiagnosticStatus status
) {
  switch (status) {
    case LED_STATUS_OK:
      return "OK";

    case LED_STATUS_OPEN:
      return "OPEN";

    case LED_STATUS_SHORT:
      return "LED SHORT";

    case LED_STATUS_OUTPUT_SHORT_GND:
      return "OUT-GND";

    case LED_STATUS_PWM_OR_UNKNOWN:
    default:
      return "CHECK";
  }
}

LedDiagnosticStatus captureSelectedChannelSid() {
  const uint16_t savedGs =
    gsValues[selectedChannel];

  const bool savedEnabled =
    channelEnabled[selectedChannel];

  uint8_t savedBc[3];
  beginDiagnosticCurrentProfile(savedBc);

  gsValues[selectedChannel] = 0x0800;
  channelEnabled[selectedChannel] = true;

  applyGsValues();

  // Several complete 12-bit PWM cycles at 1 MHz.
  delay(20);

  readSidIntoBuffer();

  const LedDiagnosticStatus status =
    classifySelectedChannelSid();

  // Restore the exact LED state and normal current profile.
  gsValues[selectedChannel] = savedGs;
  channelEnabled[selectedChannel] = savedEnabled;

  applyGsValues();
  restoreDiagnosticCurrentProfile(savedBc);

  return status;
}

void captureAllOutputSid() {
  scanOpenCount = 0;
  scanShortCount = 0;
  scanGroundCount = 0;
  scanUnknownCount = 0;

  uint8_t savedBc[3];

  /*
   * Save normal BC values and temporarily activate the
   * low-current diagnostic profile.
   */
  beginDiagnosticCurrentProfile(savedBc);

  /*
   * Start with a defined status for every TLC output.
   */
  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    allOutputStatus[outputIndex] =
      LED_STATUS_PWM_OR_UNKNOWN;
  }

  /*
   * Test only physically connected channels.
   *
   * Each channel is tested separately because this is the
   * scan method that produced correct R0, G1 and B2 SID data.
   */
  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    if (!TLC_OUTPUT_CONNECTED[outputIndex]) {
      continue;
    }

    /*
     * Set only this channel to the known diagnostic GS value.
     */
    buildDiagnosticGsSingleOutput(outputIndex);
    sendGSFrame();

    /*
     * sendGSFrame() holds BLANK LOW during transfer.
     * Re-enable the TLC outputs.
     */
    digitalWrite(PIN_BLANK, HIGH);

    /*
     * Allow several complete PWM cycles so LOD1, LOD2,
     * LSD1 and LSD2 contain fresh values.
     */
    delay(20);

    /*
     * Read the SID register using the proven routine.
     */
    readSidIntoBuffer();

    /*
     * Decode and store this channel before moving to
     * the next one.
     */
    allOutputStatus[outputIndex] =
      classifyOutputSid(outputIndex);

    switch (allOutputStatus[outputIndex]) {
      case LED_STATUS_OPEN:
        scanOpenCount++;
        break;

      case LED_STATUS_SHORT:
        scanShortCount++;
        break;

      case LED_STATUS_OUTPUT_SHORT_GND:
        scanGroundCount++;
        break;

      case LED_STATUS_PWM_OR_UNKNOWN:
        scanUnknownCount++;
        break;

      case LED_STATUS_OK:
      default:
        break;
    }
  }

  /*
   * Critical restoration sequence.
   *
   * Restore the normal GS and channel-enable values,
   * then restore the original BC values.
   */
  applyGsValues();
  restoreDiagnosticCurrentProfile(savedBc);

  /*
   * Additional safety: never finish with BLANK LOW.
   */
  digitalWrite(PIN_BLANK, HIGH);
}

void printAllOutputSid() {
  Serial.println();
  Serial.println("===== TLC SEQUENTIAL SID SCAN =====");

  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    Serial.print(TLC_OUTPUT_NAMES[outputIndex]);
    Serial.print(": ");

    if (!TLC_OUTPUT_CONNECTED[outputIndex]) {
      Serial.println("NC");
      continue;
    }

    bool lod1 = false;
    bool lod2 = false;
    bool lsd1 = false;
    bool lsd2 = false;

    getOutputDiagnosticBits(
      outputIndex,
      lod1,
      lod2,
      lsd1,
      lsd2
    );

    Serial.print(getLedDiagnosticStatusName(
      allOutputStatus[outputIndex]
    ));

    Serial.print("  [LOD1=");
    Serial.print(lod1);
    Serial.print(" LOD2=");
    Serial.print(lod2);
    Serial.print(" LSD1=");
    Serial.print(lsd1);
    Serial.print(" LSD2=");
    Serial.print(lsd2);
    Serial.println("]");
  }

  Serial.print("Connected-channel summary: OPEN=");
  Serial.print(scanOpenCount);
  Serial.print(" SHORT=");
  Serial.print(scanShortCount);
  Serial.print(" OUT-GND=");
  Serial.print(scanGroundCount);
  Serial.print(" UNKNOWN=");
  Serial.println(scanUnknownCount);
  Serial.println("==================================");
}

uint16_t getApsSidBit(uint8_t outputIndex) {
  if (outputIndex >= TLC_OUTPUT_COUNT) {
    return 0;
  }

  const uint8_t outputNumber =
    outputIndex / 3;

  const uint8_t colorGroup =
    outputIndex % 3;

  // SID APS register mapping:
  // OUTR0..OUTR7 -> bits 216..223
  // OUTG0..OUTG7 -> bits 224..231
  // OUTB0..OUTB7 -> bits 232..239
  static const uint16_t APS_BASE[3] = {
    216,  // Red
    224,  // Green
    232   // Blue
  };

  return APS_BASE[colorGroup] + outputNumber;
}

void decodeApsResult() {
  // APS_FLAG occupies SID bits 213..211.
  apsFlag =
    (static_cast<uint8_t>(getBufferBit(sidFrame, 213)) << 2) |
    (static_cast<uint8_t>(getBufferBit(sidFrame, 212)) << 1) |
    static_cast<uint8_t>(getBufferBit(sidFrame, 211));

  apsFaultCount = 0;

  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    apsOutputFault[outputIndex] =
      getBufferBit(
        sidFrame,
        getApsSidBit(outputIndex)
      );

    if (apsOutputFault[outputIndex]) {
      apsFaultCount++;
    }
  }
}

void printApsResult() {
  Serial.println();
  Serial.println("===== ADJACENT-PIN-SHORT TEST =====");

  Serial.print("APS_FLAG = 0b");
  Serial.print((apsFlag >> 2) & 0x01);
  Serial.print((apsFlag >> 1) & 0x01);
  Serial.println(apsFlag & 0x01);

  if (apsFlag == 0b011) {
    Serial.println("Result: PASS - no adjacent output pins shorted.");
  } else if (apsFlag == 0b110) {
    Serial.println("Result: FAIL - adjacent output-pin short detected.");
  } else {
    Serial.println("Result: INVALID - APS check did not return 011 or 110.");
  }

  Serial.print("APS output bits set: ");
  Serial.println(apsFaultCount);

  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    if (apsOutputFault[outputIndex]) {
      Serial.print("  ");
      Serial.println(TLC_OUTPUT_NAMES[outputIndex]);
    }
  }

  Serial.println("==================================");
}

void executeApsSpecialCommand(uint16_t command) {
  /*
   * sendSpecialCommand() leaves LATCH LOW after the 288-bit frame.
   *
   * For APS and ERROR Clear, the TLC still needs the following
   * LATCH rising edge to execute the command that was just shifted in.
   * This helper adds only that final execution edge.
   *
   * BLANK remains LOW throughout the APS procedure.
   */
  sendSpecialCommand(command);

  delayMicroseconds(20);

  digitalWrite(PIN_LATCH, HIGH);

  // APS_TIME is configured for 10 us in the current FC settings.
  // 100 us gives comfortable margin before the next operation.
  delayMicroseconds(100);
}

void runAdjacentPinTest() {
  showDiagnosticsMessage(
    "RUNNING APS TEST",
    TFT_CYAN
  );

  /*
   * TI recommends that all output channels remain OFF and
   * BLANK remain LOW during the adjacent-pin-short check.
   *
   * The software arrays are not changed. We only load a temporary
   * all-zero GS frame into the TLC, then restore the user's normal
   * GS/enable state after reading the result.
   */
  clearFrame();
  sendGSFrame();
  digitalWrite(PIN_BLANK, LOW);

  delayMicroseconds(100);

  // Clear previously latched APS/error information first.
  executeApsSpecialCommand(0xA53);

  // Execute adjacent-pin-short check.
  executeApsSpecialCommand(0x53A);

  /*
   * Read SID. APS output bits are in 239..216 and APS_FLAG
   * is in 213..211. readSidIntoBuffer() finishes by restoring
   * BLANK HIGH in this known-good baseline.
   */
  readSidIntoBuffer();
  decodeApsResult();
  printApsResult();
  queueMqttApsPublish();

  // Restore the user's normal LED pattern and safe LATCH/BLANK state.
  applyGsValues();
  digitalWrite(PIN_BLANK, HIGH);

  char message[27];

  if (apsFlag == 0b011 && apsFaultCount == 0) {
    showDiagnosticsMessage(
      "APS PASS - NO SHORT",
      TFT_GREEN
    );
    return;
  }

  if (apsFlag == 0b110) {
    const char* firstFault = nullptr;
    const char* secondFault = nullptr;

    for (
      uint8_t outputIndex = 0;
      outputIndex < TLC_OUTPUT_COUNT;
      outputIndex++
    ) {
      if (!apsOutputFault[outputIndex]) {
        continue;
      }

      if (firstFault == nullptr) {
        firstFault = TLC_OUTPUT_NAMES[outputIndex];
      } else if (secondFault == nullptr) {
        secondFault = TLC_OUTPUT_NAMES[outputIndex];
        break;
      }
    }

    if (firstFault != nullptr && secondFault != nullptr) {
      snprintf(
        message,
        sizeof(message),
        "APS SHORT %s %s",
        firstFault,
        secondFault
      );
    } else if (firstFault != nullptr) {
      snprintf(
        message,
        sizeof(message),
        "APS SHORT %s",
        firstFault
      );
    } else {
      snprintf(
        message,
        sizeof(message),
        "APS SHORT DETECTED"
      );
    }

    showDiagnosticsMessage(message, TFT_RED);
    return;
  }

  snprintf(
    message,
    sizeof(message),
    "APS INVALID %u%u%u",
    (apsFlag >> 2) & 0x01,
    (apsFlag >> 1) & 0x01,
    apsFlag & 0x01
  );

  showDiagnosticsMessage(
    message,
    TFT_YELLOW
  );
}

void decodeDeviceStatusFlags() {
  /*
   * TLC6C5724-Q1 SID global status bits:
   *   bit 215 = TEF  (thermal shutdown)
   *   bit 214 = PTW  (pre-thermal warning)
   *   bit 210 = ISF  (IREF short)
   *   bit 209 = IOF  (IREF open)
   */
  deviceTefFlag = getBufferBit(sidFrame, 215);
  devicePtwFlag = getBufferBit(sidFrame, 214);
  deviceIsfFlag = getBufferBit(sidFrame, 210);
  deviceIofFlag = getBufferBit(sidFrame, 209);
}

void printDeviceStatusFlags() {
  Serial.println();
  Serial.println("===== TLC DEVICE STATUS =====");

  Serial.print("ERR pin: ");
  Serial.println(
    isErrActive()
      ? "LOW - ACTIVE"
      : "HIGH - OK"
  );

  Serial.print("TEF thermal shutdown: ");
  Serial.println(deviceTefFlag ? "1 - ACTIVE" : "0 - OK");

  Serial.print("PTW thermal warning: ");
  Serial.println(devicePtwFlag ? "1 - ACTIVE" : "0 - OK");

  Serial.print("ISF IREF short: ");
  Serial.println(deviceIsfFlag ? "1 - ACTIVE" : "0 - OK");

  Serial.print("IOF IREF open: ");
  Serial.println(deviceIofFlag ? "1 - ACTIVE" : "0 - OK");

  Serial.println("=============================");
}

void runDeviceStatusTest() {
  showDiagnosticsMessage(
    "READING DEVICE FLAGS",
    TFT_CYAN
  );

  /*
   * Read SID using the already proven manual read routine.
   * This is a one-shot read only; there is no automatic/live scan.
   */
  readSidIntoBuffer();
  decodeDeviceStatusFlags();
  printDeviceStatusFlags();

  // Restore the user's normal GS pattern and safe output state.
  applyGsValues();
  digitalWrite(PIN_BLANK, HIGH);

  updateDiagnosticsErrStatus(true);
  queueMqttDeviceStatusPublish();

  /*
   * Display the highest-priority active device fault.
   * All individual flag values remain visible in Serial Monitor.
   */
  if (deviceTefFlag) {
    showDiagnosticsMessage(
      "THERMAL SHUTDOWN",
      TFT_RED
    );
    return;
  }

  if (devicePtwFlag) {
    showDiagnosticsMessage(
      "THERMAL WARNING",
      TFT_ORANGE
    );
    return;
  }

  if (deviceIsfFlag) {
    showDiagnosticsMessage(
      "IREF SHORT",
      TFT_RED
    );
    return;
  }

  if (deviceIofFlag) {
    showDiagnosticsMessage(
      "IREF OPEN",
      TFT_RED
    );
    return;
  }

  showDiagnosticsMessage(
    "DEVICE FLAGS OK",
    TFT_GREEN
  );
}


void decodeLodLsdSelfTestFlag() {
  /*
   * SID bits 208..206 contain LOD_LSD_FLAG:
   *   011 = detector-circuit self-test passed
   *   110 = detector-circuit self-test failed
   */
  lodLsdSelfTestFlag =
    (static_cast<uint8_t>(getBufferBit(sidFrame, 208)) << 2) |
    (static_cast<uint8_t>(getBufferBit(sidFrame, 207)) << 1) |
     static_cast<uint8_t>(getBufferBit(sidFrame, 206));
}

void printLodLsdSelfTestResult() {
  Serial.println();
  Serial.println("===== LOD/LSD CIRCUIT SELF-TEST =====");

  Serial.print("LOD_LSD_FLAG = 0b");
  Serial.print((lodLsdSelfTestFlag >> 2) & 0x01);
  Serial.print((lodLsdSelfTestFlag >> 1) & 0x01);
  Serial.println(lodLsdSelfTestFlag & 0x01);

  if (lodLsdSelfTestFlag == 0b011) {
    Serial.println(
      "Result: PASS - LOD/LSD detector circuits are operational."
    );
  }
  else if (lodLsdSelfTestFlag == 0b110) {
    Serial.println(
      "Result: FAIL - LOD/LSD detector-circuit fault detected."
    );
  }
  else {
    Serial.println(
      "Result: INVALID - self-test did not return 011 or 110."
    );
  }

  Serial.print("ERR pin: ");
  Serial.println(
    isErrActive()
      ? "LOW - ACTIVE"
      : "HIGH - OK"
  );

  Serial.println("=====================================");
}

void runLodLsdSelfTest() {
  showDiagnosticsMessage(
    "RUNNING LOD/LSD TEST",
    TFT_CYAN
  );

  /*
   * Datasheet requirement:
   * Keep BLANK LOW while command 0x535 executes.
   *
   * The special-command helper adds the final LATCH
   * rising edge which executes the command.
   */
  digitalWrite(PIN_BLANK, LOW);
  delayMicroseconds(20);

  executeApsSpecialCommand(0x535);

  /*
   * Read SID after the self-test:
   *   bits 208..206 = LOD_LSD_FLAG
   */
  readSidIntoBuffer();
  decodeLodLsdSelfTestFlag();
  printLodLsdSelfTestResult();

  // Restore the user's normal LED state.
  applyGsValues();
  digitalWrite(PIN_BLANK, HIGH);
  delay(5);

  updateDiagnosticsErrStatus(true);
  queueMqttLodLsdPublish();

  if (lodLsdSelfTestFlag == 0b011) {
    showDiagnosticsMessage(
      "LOD/LSD TEST PASS",
      TFT_GREEN
    );
    return;
  }

  if (lodLsdSelfTestFlag == 0b110) {
    showDiagnosticsMessage(
      "LOD/LSD TEST FAIL",
      TFT_RED
    );
    return;
  }

  char message[27];

  snprintf(
    message,
    sizeof(message),
    "LOD/LSD INVALID %u%u%u",
    (lodLsdSelfTestFlag >> 2) & 0x01,
    (lodLsdSelfTestFlag >> 1) & 0x01,
    lodLsdSelfTestFlag & 0x01
  );

  showDiagnosticsMessage(
    message,
    TFT_YELLOW
  );
}


bool isNegateControlledSidBit(uint16_t bit) {
  /*
   * NEG2 controls:
   *   LOD2 bits 287..264
   *   LSD2 bits 191..168
   *
   * NEG1 controls:
   *   LOD1 bits 263..240
   *   LSD1 bits 167..144
   */
  return
    (bit >= 264 && bit <= 287) ||
    (bit >= 240 && bit <= 263) ||
    (bit >= 168 && bit <= 191) ||
    (bit >= 144 && bit <= 167);
}

int8_t getNegateOutputIndexForSidBit(uint16_t bit) {
  struct SidRange {
    uint16_t base;
    uint8_t colourOffset;
  };

  /*
   * Output array order:
   * R0, G0, B0, R1, G1, B1, ... R7, G7, B7
   */
  static const SidRange ranges[] = {
    {240, 0},  // LOD1 R0..R7
    {248, 1},  // LOD1 G0..G7
    {256, 2},  // LOD1 B0..B7
    {264, 0},  // LOD2 R0..R7
    {272, 1},  // LOD2 G0..G7
    {280, 2},  // LOD2 B0..B7
    {144, 0},  // LSD1 R0..R7
    {152, 1},  // LSD1 G0..G7
    {160, 2},  // LSD1 B0..B7
    {168, 0},  // LSD2 R0..R7
    {176, 1},  // LSD2 G0..G7
    {184, 2}   // LSD2 B0..B7
  };

  for (const SidRange& range : ranges) {
    if (bit >= range.base && bit < range.base + 8U) {
      const uint8_t channelNumber =
        static_cast<uint8_t>(bit - range.base);

      return static_cast<int8_t>(
        channelNumber * 3U + range.colourOffset
      );
    }
  }

  return -1;
}

const char* getNegateRegisterName(uint16_t bit) {
  if (bit >= 240 && bit <= 263) {
    return "LOD1";
  }

  if (bit >= 264 && bit <= 287) {
    return "LOD2";
  }

  if (bit >= 144 && bit <= 167) {
    return "LSD1";
  }

  if (bit >= 168 && bit <= 191) {
    return "LSD2";
  }

  return "UNKNOWN";
}

bool isConnectedNegateSidBit(uint16_t bit) {
  const int8_t outputIndex =
    getNegateOutputIndexForSidBit(bit);

  if (outputIndex < 0) {
    return false;
  }

  return TLC_OUTPUT_CONNECTED[outputIndex];
}

uint16_t getConnectedNegateRegisterBitCount() {
  uint16_t connectedOutputCount = 0;

  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    if (TLC_OUTPUT_CONNECTED[outputIndex]) {
      connectedOutputCount++;
    }
  }

  /*
   * Each physically connected output contributes four bits:
   * LOD1, LOD2, LSD1 and LSD2.
   */
  return connectedOutputCount * 4U;
}

void buildNegGclkDiagnosticGsAllOutputs() {
  /*
   * Deterministic NEG/GCLK diagnostic condition:
   * all 24 outputs are OFF for the complete PWM cycle.
   *
   * With a channel off before the 9th GCLK, the datasheet's
   * Table 6 gives a stable detector state:
   *   LOD1 = 0, LSD1 = 1
   *   LOD2 = 0, LSD2 = 1
   *
   * This also avoids relying on floating unused outputs while
   * they are actively driven.
   *
   * The user's gsValues[] array is not modified.
   */
  clearFrame();
}


uint16_t countNegateInversionMatches(
  const uint8_t* beforeFrame,
  const uint8_t* afterFrame
) {
  uint16_t matchCount = 0;

  for (uint16_t bit = 144; bit <= 287; bit++) {
    if (!isNegateControlledSidBit(bit)) {
      continue;
    }

    const bool beforeValue =
      getBufferBit(beforeFrame, bit);

    const bool afterValue =
      getBufferBit(afterFrame, bit);

    if (afterValue == !beforeValue) {
      matchCount++;
    }
  }

  return matchCount;
}

uint16_t countNegateEqualityMatches(
  const uint8_t* firstFrame,
  const uint8_t* secondFrame
) {
  uint16_t matchCount = 0;

  for (uint16_t bit = 144; bit <= 287; bit++) {
    if (!isNegateControlledSidBit(bit)) {
      continue;
    }

    if (
      getBufferBit(firstFrame, bit) ==
      getBufferBit(secondFrame, bit)
    ) {
      matchCount++;
    }
  }

  return matchCount;
}

void printNegateMismatchedBits(
  const uint8_t* beforeFrame,
  const uint8_t* afterFrame
) {
  Serial.print("Non-inverting SID bits:");

  uint8_t printed = 0;

  for (int bit = 287; bit >= 144; bit--) {
    if (!isNegateControlledSidBit(
          static_cast<uint16_t>(bit))) {
      continue;
    }

    const bool beforeValue =
      getBufferBit(
        beforeFrame,
        static_cast<uint16_t>(bit)
      );

    const bool afterValue =
      getBufferBit(
        afterFrame,
        static_cast<uint16_t>(bit)
      );

    if (afterValue != !beforeValue) {
      Serial.print(" ");
      Serial.print(bit);
      printed++;
    }
  }

  if (printed == 0) {
    Serial.print(" none");
  }

  Serial.println();
}

void printIgnoredNcNegateMismatches(
  const uint8_t* beforeFrame,
  const uint8_t* afterFrame
) {
  Serial.print("Ignored NC mismatches:");

  uint8_t printed = 0;

  for (int bit = 287; bit >= 144; bit--) {
    const uint16_t sidBit =
      static_cast<uint16_t>(bit);

    if (!isNegateControlledSidBit(sidBit)) {
      continue;
    }

    const int8_t outputIndex =
      getNegateOutputIndexForSidBit(sidBit);

    if (
      outputIndex < 0 ||
      TLC_OUTPUT_CONNECTED[outputIndex]
    ) {
      continue;
    }

    const bool beforeValue =
      getBufferBit(beforeFrame, sidBit);

    const bool afterValue =
      getBufferBit(afterFrame, sidBit);

    if (afterValue != !beforeValue) {
      Serial.print(" ");
      Serial.print(getNegateRegisterName(sidBit));
      Serial.print(":");
      Serial.print(TLC_OUTPUT_NAMES[outputIndex]);
      Serial.print("(bit");
      Serial.print(sidBit);
      Serial.print(")");
      printed++;
    }
  }

  if (printed == 0) {
    Serial.print(" none");
  }

  Serial.println();
}

void runNegGclkIntegrityTest() {
  uint8_t beforeFrame[TLC_FRAME_BYTES];
  uint8_t toggledFrame[TLC_FRAME_BYTES];
  uint8_t restoredFrame[TLC_FRAME_BYTES];

  const uint16_t connectedRegisterBitCount =
    getConnectedNegateRegisterBitCount();

  showDiagnosticsMessage(
    "RUNNING NEG/GCLK",
    TFT_CYAN
  );

  /*
   * Apply a deterministic all-off GS frame to all 24 outputs.
   * This removes LED-current and unused-output variation from
   * the NEG register comparison.
   */
  buildNegGclkDiagnosticGsAllOutputs();
  sendGSFrame();
  digitalWrite(PIN_BLANK, HIGH);

  // Allow several complete 12-bit GS cycles at 1 MHz.
  delay(NEG_GCLK_WAIT_MS);

  readSidIntoBuffer();
  memcpy(beforeFrame, sidFrame, sizeof(beforeFrame));

  const bool neg1Before =
    getBufferBit(beforeFrame, 205);

  const bool neg2Before =
    getBufferBit(beforeFrame, 204);

  /*
   * Toggle NEG1 and NEG2. The datasheet requires waiting at
   * least one complete GS counter cycle before reading SID.
   */
  digitalWrite(PIN_BLANK, LOW);
  delayMicroseconds(20);
  executeApsSpecialCommand(0x55A);

  digitalWrite(PIN_BLANK, HIGH);
  delay(NEG_GCLK_WAIT_MS);

  readSidIntoBuffer();
  memcpy(toggledFrame, sidFrame, sizeof(toggledFrame));

  const bool neg1After =
    getBufferBit(toggledFrame, 205);

  const bool neg2After =
    getBufferBit(toggledFrame, 204);

  const bool negatePairConsistent =
    (neg1Before == neg2Before) &&
    (neg1After == neg2After);

  const bool negBitsToggled =
    (neg1After == !neg1Before) &&
    (neg2After == !neg2Before);

  negInversionMatchCount =
    countNegateInversionMatches(
      beforeFrame,
      toggledFrame
    );

  negConnectedInversionMatchCount =
    countConnectedNegateInversionMatches(
      beforeFrame,
      toggledFrame
    );

  /*
   * Full-register comparison is retained as diagnostic
   * information. The PASS criterion uses every output marked
   * true in TLC_OUTPUT_CONNECTED[]. Floating NC outputs are
   * ignored until they are physically populated and enabled.
   */
  const bool connectedLodLsdBitsInverted =
    negConnectedInversionMatchCount ==
    connectedRegisterBitCount;

  /*
   * Toggle a second time and verify both the NEG bits and all
   * 96 LOD/LSD register bits return to the baseline state.
   */
  digitalWrite(PIN_BLANK, LOW);
  delayMicroseconds(20);
  executeApsSpecialCommand(0x55A);

  digitalWrite(PIN_BLANK, HIGH);
  delay(NEG_GCLK_WAIT_MS);

  readSidIntoBuffer();
  memcpy(restoredFrame, sidFrame, sizeof(restoredFrame));

  const bool neg1Restored =
    getBufferBit(restoredFrame, 205);

  const bool neg2Restored =
    getBufferBit(restoredFrame, 204);

  const bool negStateRestored =
    (neg1Restored == neg1Before) &&
    (neg2Restored == neg2Before);

  const uint16_t restoreMatchCount =
    countNegateEqualityMatches(
      beforeFrame,
      restoredFrame
    );

  const uint16_t connectedRestoreMatchCount =
    countConnectedNegateEqualityMatches(
      beforeFrame,
      restoredFrame
    );

  const bool connectedLodLsdBitsRestored =
    connectedRestoreMatchCount ==
    connectedRegisterBitCount;

  mqttNegConnectedExpected =
    connectedRegisterBitCount;

  mqttNegConnectedRestoreMatches =
    connectedRestoreMatchCount;

  mqttNegAllRestoreMatches =
    restoreMatchCount;

  negGclkTestPassed =
    negatePairConsistent &&
    negBitsToggled &&
    connectedLodLsdBitsInverted &&
    negStateRestored &&
    connectedLodLsdBitsRestored;

  Serial.println();
  Serial.println("===== NEG/GCLK INTEGRITY TEST =====");
  Serial.println("Diagnostic pattern: all 24 outputs OFF");

  Serial.print("Connected outputs under test: ");
  Serial.println(connectedRegisterBitCount / 4U);

  Serial.print("Connected register bits expected: ");
  Serial.println(connectedRegisterBitCount);

  Serial.print("NEG before:  NEG1=");
  Serial.print(neg1Before);
  Serial.print(" NEG2=");
  Serial.println(neg2Before);

  Serial.print("NEG toggled: NEG1=");
  Serial.print(neg1After);
  Serial.print(" NEG2=");
  Serial.println(neg2After);

  Serial.print("All-output inversion matches: ");
  Serial.print(negInversionMatchCount);
  Serial.print("/");
  Serial.println(NEG_REGISTER_BIT_COUNT);

  Serial.print("Connected-channel inversion matches: ");
  Serial.print(negConnectedInversionMatchCount);
  Serial.print("/");
  Serial.println(connectedRegisterBitCount);

  if (negInversionMatchCount != NEG_REGISTER_BIT_COUNT) {
    printIgnoredNcNegateMismatches(
      beforeFrame,
      toggledFrame
    );
  }

  Serial.print("NEG restored: NEG1=");
  Serial.print(neg1Restored);
  Serial.print(" NEG2=");
  Serial.println(neg2Restored);

  Serial.print("All-output restoration matches: ");
  Serial.print(restoreMatchCount);
  Serial.print("/");
  Serial.println(NEG_REGISTER_BIT_COUNT);

  Serial.print("Connected-channel restoration matches: ");
  Serial.print(connectedRestoreMatchCount);
  Serial.print("/");
  Serial.println(connectedRegisterBitCount);

  Serial.print("NEG pair consistency: ");
  Serial.println(
    negatePairConsistent ? "PASS" : "FAIL"
  );

  Serial.print("NEG toggle: ");
  Serial.println(
    negBitsToggled ? "PASS" : "FAIL"
  );

  Serial.print("Connected GCLK/register inversion: ");
  Serial.println(
    connectedLodLsdBitsInverted ? "PASS" : "FAIL"
  );

  Serial.print("NEG restore: ");
  Serial.println(
    negStateRestored ? "PASS" : "FAIL"
  );

  Serial.print("Connected register restore: ");
  Serial.println(
    connectedLodLsdBitsRestored ? "PASS" : "FAIL"
  );

  Serial.print("Overall result: ");
  Serial.println(
    negGclkTestPassed ? "PASS" : "FAIL"
  );

  Serial.println("===================================");

  /*
   * Restore the user's normal grayscale data. BC and DC were
   * never changed by this test.
   */
  applyGsValues();

  digitalWrite(PIN_BLANK, HIGH);
  delay(5);

  updateDiagnosticsErrStatus(true);
  queueMqttNegGclkPublish();

  if (negGclkTestPassed) {
    showDiagnosticsMessage(
      "NEG/GCLK TEST PASS",
      TFT_GREEN
    );
    return;
  }

  if (!negBitsToggled) {
    showDiagnosticsMessage(
      "NEG TOGGLE FAIL",
      TFT_RED
    );
    return;
  }

  if (!connectedLodLsdBitsInverted) {
    showDiagnosticsMessage(
      "GCLK/REG TEST FAIL",
      TFT_RED
    );
    return;
  }

  if (
    !negStateRestored ||
    !connectedLodLsdBitsRestored
  ) {
    showDiagnosticsMessage(
      "NEG RESTORE FAIL",
      TFT_RED
    );
    return;
  }

  showDiagnosticsMessage(
    "NEG/GCLK TEST FAIL",
    TFT_RED
  );
}

uint8_t getSidFieldByte(uint16_t msb) {
  if (msb < 7 || msb > 287) {
    return 0;
  }

  uint8_t value = 0;

  for (uint8_t offset = 0; offset < 8; offset++) {
    const uint16_t commonBit = msb - offset;

    if (getBufferBit(sidFrame, commonBit)) {
      value |= static_cast<uint8_t>(
        1U << (7 - offset)
      );
    }
  }

  return value;
}

void printByteBinary(uint8_t value) {
  for (int8_t bit = 7; bit >= 0; bit--) {
    Serial.print((value >> bit) & 0x01);
  }
}

void printSidRawFrame() {
  Serial.println("Raw SID bytes for common bits 287..144:");

  for (uint8_t byteIndex = 0; byteIndex < 18; byteIndex++) {
    const uint16_t msb =
      287 - static_cast<uint16_t>(byteIndex) * 8U;

    const uint16_t lsb = msb - 7;
    const uint8_t value = getSidFieldByte(msb);

    Serial.print("bits ");
    Serial.print(msb);
    Serial.print("..");
    Serial.print(lsb);
    Serial.print(" = 0x");

    if (value < 0x10) {
      Serial.print("0");
    }

    Serial.print(value, HEX);
    Serial.print("  ");
    printByteBinary(value);
    Serial.println();
  }
}

void printSidGroupFields() {
  struct SidField {
    const char* name;
    uint16_t msb;
  };

  const SidField fields[] = {
    {"LOD2_B", 287},
    {"LOD2_G", 279},
    {"LOD2_R", 271},

    {"LOD1_B", 263},
    {"LOD1_G", 255},
    {"LOD1_R", 247},

    {"LSD2_B", 191},
    {"LSD2_G", 183},
    {"LSD2_R", 175},

    {"LSD1_B", 167},
    {"LSD1_G", 159},
    {"LSD1_R", 151}
  };

  Serial.println("Decoded 8-bit SID fields:");

  for (
    size_t index = 0;
    index < sizeof(fields) / sizeof(fields[0]);
    index++
  ) {
    const uint8_t value =
      getSidFieldByte(fields[index].msb);

    Serial.print(fields[index].name);
    Serial.print(" = 0x");

    if (value < 0x10) {
      Serial.print("0");
    }

    Serial.print(value, HEX);
    Serial.print("  ");
    printByteBinary(value);
    Serial.println();
  }
}

void printSidBitWindow(
  const char* label,
  uint16_t centerBit
) {
  Serial.print(label);
  Serial.print(" around bit ");
  Serial.print(centerBit);
  Serial.print(": ");

  for (int8_t offset = 3; offset >= -3; offset--) {
    const int16_t candidateBit =
      static_cast<int16_t>(centerBit) + offset;

    if (candidateBit < 0 || candidateBit > 287) {
      Serial.print("x");
    } else {
      Serial.print(
        getBufferBit(
          sidFrame,
          static_cast<uint16_t>(candidateBit)
        )
      );
    }
  }

  Serial.println("  (+3 ... -3)");
}

void runSidRawDebugForOutput(uint8_t outputIndex) {
  if (outputIndex >= CHANNEL_COUNT) {
    return;
  }

  buildDiagnosticGsSingleOutput(outputIndex);
  sendGSFrame();
  digitalWrite(PIN_BLANK, HIGH);

  // Allow several complete 12-bit PWM cycles.
  delay(20);

  readSidIntoBuffer();

  Serial.println();
  Serial.println("========================================");
  Serial.print("SID RAW DEBUG TARGET: ");
  Serial.println(TLC_OUTPUT_NAMES[outputIndex]);
  Serial.println("========================================");

  printSidRawFrame();
  Serial.println();
  printSidGroupFields();

  bool lod1 = false;
  bool lod2 = false;
  bool lsd1 = false;
  bool lsd2 = false;

  getOutputDiagnosticBits(
    outputIndex,
    lod1,
    lod2,
    lsd1,
    lsd2
  );

  Serial.println();
  Serial.print("Current decoder result for ");
  Serial.print(TLC_OUTPUT_NAMES[outputIndex]);
  Serial.print(": LOD1=");
  Serial.print(lod1);
  Serial.print(" LOD2=");
  Serial.print(lod2);
  Serial.print(" LSD1=");
  Serial.print(lsd1);
  Serial.print(" LSD2=");
  Serial.print(lsd2);
  Serial.print(" -> ");
  Serial.println(
    getLedDiagnosticStatusName(
      classifyOutputSid(outputIndex)
    )
  );

  const uint8_t outputNumber = outputIndex / 3;
  const uint8_t colorGroup = outputIndex % 3;

  static const uint16_t LOD2_BASE[3] = {
    264, 272, 280
  };

  static const uint16_t LOD1_BASE[3] = {
    240, 248, 256
  };

  static const uint16_t LSD2_BASE[3] = {
    168, 176, 184
  };

  static const uint16_t LSD1_BASE[3] = {
    144, 152, 160
  };

  Serial.println("Expected-bit neighbourhoods:");

  printSidBitWindow(
    "LOD2",
    LOD2_BASE[colorGroup] + outputNumber
  );

  printSidBitWindow(
    "LOD1",
    LOD1_BASE[colorGroup] + outputNumber
  );

  printSidBitWindow(
    "LSD2",
    LSD2_BASE[colorGroup] + outputNumber
  );

  printSidBitWindow(
    "LSD1",
    LSD1_BASE[colorGroup] + outputNumber
  );
}

void runSidRawDebug() {
  Serial.println();
  Serial.println("########################################");
  Serial.println("STARTING SID RAW DEBUG");
  Serial.println("Targets: R0, G1 and B2");
  Serial.println("Keep all LED branches connected.");
  Serial.println("########################################");

  uint8_t savedBc[3];
  beginDiagnosticCurrentProfile(savedBc);

  runSidRawDebugForOutput(0);  // R0
  runSidRawDebugForOutput(4);  // G1
  runSidRawDebugForOutput(8);  // B2

  applyGsValues();
  restoreDiagnosticCurrentProfile(savedBc);

  Serial.println();
  Serial.println("########################################");
  Serial.println("SID RAW DEBUG COMPLETE");
  Serial.println("########################################");
}

void printSelectedChannelSid(
  LedDiagnosticStatus status
) {
  bool lod1 = false;
  bool lod2 = false;
  bool lsd1 = false;
  bool lsd2 = false;

  getSelectedChannelDiagnosticBits(
    lod1,
    lod2,
    lsd1,
    lsd2
  );

  Serial.print("SID ");
  Serial.print(getSelectedChannelName());
  Serial.print(": LOD1=");
  Serial.print(lod1);
  Serial.print(" LOD2=");
  Serial.print(lod2);
  Serial.print(" LSD1=");
  Serial.print(lsd1);
  Serial.print(" LSD2=");
  Serial.print(lsd2);
  Serial.print(" -> ");
  Serial.println(getLedDiagnosticStatusName(status));

  Serial.print("Global SID flags: TEF=");
  Serial.print(getBufferBit(sidFrame, 215));
  Serial.print(" PTW=");
  Serial.print(getBufferBit(sidFrame, 214));
  Serial.print(" ISF=");
  Serial.print(getBufferBit(sidFrame, 210));
  Serial.print(" IOF=");
  Serial.println(getBufferBit(sidFrame, 209));
}

void runSelectedDiagnostic() {
  switch (selectedOption) {
    case 0:
      runNegGclkIntegrityTest();
      break;

    case 1: {
      Serial.println();
      Serial.println("===== ERROR CLEAR =====");
      Serial.println("Executing command 0xA53 with final LATCH edge...");

      showDiagnosticsMessage(
        "CLEARING ERRORS",
        TFT_CYAN
      );

      // Keep outputs blanked while the command is executed.
      digitalWrite(PIN_BLANK, LOW);
      delayMicroseconds(20);

      // IMPORTANT:
      // sendSpecialCommand() alone finishes with LATCH LOW.
      // The extra rising edge in this helper is the edge that
      // executes ERROR CLEAR and resets the latched error registers.
      executeApsSpecialCommand(0xA53);

      // Give the open-drain ERR output time to release after the
      // internal error register has been cleared.
      delay(2);

      // Read a fresh SID snapshot after the clear command.
      // ERROR CLEAR itself places the old status in the common register,
      // so the explicit SID-read command below is required to obtain
      // the new post-clear state.
      readSidIntoBuffer();
      decodeDeviceStatusFlags();

      // Restore the user's LED pattern and safe idle state.
      applyGsValues();
      digitalWrite(PIN_BLANK, HIGH);
      delay(5);

      const bool errStillActive = isErrActive();

      Serial.print("ERR after clear: ");
      Serial.println(
        errStillActive
          ? "LOW - FAULT STILL ACTIVE"
          : "HIGH - CLEARED"
      );

      Serial.print("Post-clear flags: TEF=");
      Serial.print(deviceTefFlag);
      Serial.print(" PTW=");
      Serial.print(devicePtwFlag);
      Serial.print(" ISF=");
      Serial.print(deviceIsfFlag);
      Serial.print(" IOF=");
      Serial.println(deviceIofFlag);
      Serial.println("=======================");

      updateDiagnosticsErrStatus(true);

      const bool globalFaultStillPresent =
        deviceTefFlag ||
        devicePtwFlag ||
        deviceIsfFlag ||
        deviceIofFlag;

      const bool clearSucceeded =
        !errStillActive &&
        !globalFaultStillPresent;

      queueMqttErrorClearPublish(
        clearSucceeded,
        errStillActive
      );

      if (deviceTefFlag) {
        showDiagnosticsMessage(
          "THERMAL SHUTDOWN",
          TFT_RED
        );
      }
      else if (devicePtwFlag) {
        showDiagnosticsMessage(
          "THERMAL WARNING",
          TFT_ORANGE
        );
      }
      else if (deviceIsfFlag) {
        showDiagnosticsMessage(
          "IREF SHORT",
          TFT_RED
        );
      }
      else if (deviceIofFlag) {
        showDiagnosticsMessage(
          "IREF OPEN",
          TFT_RED
        );
      }
      else if (errStillActive || globalFaultStillPresent) {
        showDiagnosticsMessage(
          "ERROR STILL ACTIVE",
          TFT_RED
        );
      }
      else {
        showDiagnosticsMessage(
          "ERRORS CLEARED",
          TFT_GREEN
        );
      }

      break;
    }

    case 2:
      runAdjacentPinTest();
      break;

    case 3: {
      showDiagnosticsMessage(
        "SCANNING CHANNELS",
        TFT_CYAN
      );

      captureAllOutputSid();
      printAllOutputSid();
      queueMqttChannelScanPublish();

      diagnosticsGridVisible = true;
      drawDiagnosticsGrid();
      break;
    }

    case 4:
      runDeviceStatusTest();
      break;

    case 5:
      runLodLsdSelfTest();
      break;

    default:
      break;
  }
}
