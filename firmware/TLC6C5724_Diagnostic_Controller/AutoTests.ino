const char* getAutomaticTestName(
  AutomaticTest test
) {
  switch (test) {
    case AUTO_TEST_RGB_CYCLE:
      return "rgb_cycle";

    case AUTO_TEST_RGB_PHASE_ROTATION:
      return "rgb_phase_rotation";

    case AUTO_TEST_LED_CHASE:
      return "led_chase";

    case AUTO_TEST_BRIGHTNESS_SWEEP:
      return "brightness_sweep";

    case AUTO_TEST_ALL_WHITE:
      return "all_white";

    case AUTO_TEST_ALL_OFF:
      return "all_off";

    case AUTO_TEST_NONE:
    default:
      return "none";
  }
}

const char* getAutomaticTestLabel(
  AutomaticTest test
) {
  switch (test) {
    case AUTO_TEST_RGB_CYCLE:
      return "RGB CYCLE";

    case AUTO_TEST_RGB_PHASE_ROTATION:
      return "RGB PHASE ROTATION";

    case AUTO_TEST_LED_CHASE:
      return "LED CHASE";

    case AUTO_TEST_BRIGHTNESS_SWEEP:
      return "BRIGHTNESS SWEEP";

    case AUTO_TEST_ALL_WHITE:
      return "ALL WHITE";

    case AUTO_TEST_ALL_OFF:
      return "ALL OFF";

    case AUTO_TEST_NONE:
    default:
      return "STOPPED";
  }
}

void showAutomaticTestMessage(
  const char* message,
  uint16_t color
) {
  if (currentPage != PAGE_TESTS) {
    return;
  }

  tft.fillRect(0, 98, 160, 10, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(color, TFT_BLACK);
  tft.setCursor(5, 100);
  tft.print(message);
}

void buildAutomaticRgbFrame(
  uint16_t red,
  uint16_t green,
  uint16_t blue
) {
  clearFrame();

  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    if (!TLC_OUTPUT_CONNECTED[outputIndex]) {
      continue;
    }

    uint16_t value = 0;

    switch (outputIndex % 3U) {
      case 0:
        value = red;
        break;

      case 1:
        value = green;
        break;

      case 2:
        value = blue;
        break;
    }

    if (value > 0x0FFF) {
      value = 0x0FFF;
    }

    const uint16_t lsb =
      static_cast<uint16_t>(outputIndex) * 12U;

    setFieldTLC(
      lsb + 11,
      lsb,
      value
    );
  }
}

void buildAutomaticWhiteFrame(
  uint16_t level
) {
  clearFrame();

  if (level > 0x0FFF) {
    level = 0x0FFF;
  }

  for (
    uint8_t outputIndex = 0;
    outputIndex < TLC_OUTPUT_COUNT;
    outputIndex++
  ) {
    if (!TLC_OUTPUT_CONNECTED[outputIndex]) {
      continue;
    }

    const uint16_t lsb =
      static_cast<uint16_t>(outputIndex) * 12U;

    setFieldTLC(
      lsb + 11,
      lsb,
      level
    );
  }
}

void buildAutomaticPhaseRotationFrame(
  uint8_t phase,
  uint16_t level
) {
  clearFrame();

  if (level > 0x0FFF) {
    level = 0x0FFF;
  }

  phase %= 3U;

  // Rotate R/G/B assignments across the three connected RGB positions.
  for (uint8_t ledIndex = 0; ledIndex < 3U; ledIndex++) {
    const uint8_t colourIndex =
      static_cast<uint8_t>((ledIndex + phase) % 3U);

    const uint8_t outputIndex =
      ledIndex * 3U + colourIndex;

    if (!TLC_OUTPUT_CONNECTED[outputIndex]) {
      continue;
    }

    const uint16_t lsb =
      static_cast<uint16_t>(outputIndex) * 12U;

    setFieldTLC(
      lsb + 11,
      lsb,
      level
    );
  }
}

void buildAutomaticChaseFrame(
  uint8_t ledIndex,
  uint8_t colourIndex,
  uint16_t level
) {
  clearFrame();

  if (level > 0x0FFF) {
    level = 0x0FFF;
  }

  if (ledIndex >= 3 || colourIndex >= 3) {
    return;
  }

  // outputIndex = RGB group * 3 + colour index.

  const uint8_t outputIndex =
    ledIndex * 3U + colourIndex;

  const uint16_t lsb =
    static_cast<uint16_t>(outputIndex) * 12U;

  setFieldTLC(
    lsb + 11,
    lsb,
    level
  );
}

void applyAutomaticTestFrame() {
  sendGSFrameFast();
  digitalWrite(PIN_BLANK, HIGH);
}

void renderAutomaticTestStep() {
  switch (activeAutomaticTest) {
    case AUTO_TEST_RGB_CYCLE: {
      // Seven visually distinct RGB combinations.
      static const uint8_t sequence[7][3] = {
        {1, 0, 0},  // red
        {0, 1, 0},  // green
        {0, 0, 1},  // blue
        {1, 1, 0},  // yellow
        {0, 1, 1},  // cyan
        {1, 0, 1},  // magenta
        {1, 1, 1}   // white
      };

      const uint8_t index =
        automaticTestStep % 7U;

      buildAutomaticRgbFrame(
        sequence[index][0] ? DEMO_TEST_GS : 0,
        sequence[index][1] ? DEMO_TEST_GS : 0,
        sequence[index][2] ? DEMO_TEST_GS : 0
      );

      applyAutomaticTestFrame();
      break;
    }


    case AUTO_TEST_RGB_PHASE_ROTATION:
      buildAutomaticPhaseRotationFrame(
        automaticTestStep % 3U,
        DEMO_TEST_GS
      );
      applyAutomaticTestFrame();
      break;

    case AUTO_TEST_LED_CHASE: {
      // Nine-step colour chase:
  //
  // LED0 Red
  // LED1 Green
  // LED2 Blue
  // LED0 Green
  // LED1 Blue
  // LED2 Red
  // LED0 Blue
  // LED1 Red
  // LED2 Green

      static const uint8_t chaseSequence[9][2] = {
  {0, 0},  // LED0 Red
  {1, 0},  // LED1 Red
  {2, 0},  // LED2 Red

  {0, 1},  // LED0 Green
  {1, 1},  // LED1 Green
  {2, 1},  // LED2 Green

  {0, 2},  // LED0 Blue
  {1, 2},  // LED1 Blue
  {2, 2}   // LED2 Blue
};

      const uint8_t index =
        automaticTestStep % 9U;

      buildAutomaticChaseFrame(
    chaseSequence[index][0],
    chaseSequence[index][1],
    DEMO_TEST_GS
  );

      applyAutomaticTestFrame();
      break;
    }

    case AUTO_TEST_BRIGHTNESS_SWEEP: {
  // Square the 0..255 step for a smoother low-brightness ramp.
      const uint32_t step =
    static_cast<uint32_t>(automaticTestStep);

      const uint16_t level =
    static_cast<uint16_t>(
      (step * step * 4095UL) /
      (255UL * 255UL)
    );

      buildAutomaticWhiteFrame(level);
      applyAutomaticTestFrame();
      break;
    }

    case AUTO_TEST_ALL_WHITE:
      buildAutomaticWhiteFrame(DEMO_TEST_GS);
      applyAutomaticTestFrame();
      break;

    case AUTO_TEST_ALL_OFF:
      clearFrame();
      applyAutomaticTestFrame();
      break;

    case AUTO_TEST_NONE:
    default:
      break;
  }
}

void startAutomaticTest(
  AutomaticTest test
) {
  if (test == AUTO_TEST_NONE) {
    stopAutomaticTest(true);
    return;
  }

  activeAutomaticTest = test;
  automaticTestStep = 0;
  automaticBrightnessDirection = 1;
  previousAutomaticTestStepTime = millis();

  currentPage = PAGE_TESTS;

  if (
    test >= AUTO_TEST_RGB_CYCLE &&
    test <= AUTO_TEST_ALL_OFF
  ) {
    selectedOption =
      static_cast<uint8_t>(test) - 1U;
  }

  drawTestsPage();
  renderAutomaticTestStep();

  char message[28];
  snprintf(
    message,
    sizeof(message),
    "RUN: %s",
    getAutomaticTestLabel(test)
  );

  showAutomaticTestMessage(
    message,
    TFT_CYAN
  );

  Serial.print("Automatic test started: ");
  Serial.println(getAutomaticTestName(test));

  queueMqttAutomaticTestStatus(
    getAutomaticTestName(test),
    "running"
  );
}

void stopAutomaticTest(
  bool restoreManualState
) {
  const AutomaticTest previousTest =
    activeAutomaticTest;

  activeAutomaticTest = AUTO_TEST_NONE;
  automaticTestStep = 0;
  automaticBrightnessDirection = 1;
  previousAutomaticTestStepTime = millis();

  if (restoreManualState) {
    applyGsValuesFast();
  }

  if (previousTest != AUTO_TEST_NONE) {
    Serial.print("Automatic test stopped: ");
    Serial.println(
      getAutomaticTestName(previousTest)
    );
  }

  queueMqttAutomaticTestStatus(
    previousTest == AUTO_TEST_NONE
      ? "none"
      : getAutomaticTestName(previousTest),
    "stopped"
  );

  showAutomaticTestMessage(
    "TEST STOPPED",
    TFT_GREEN
  );
}

void serviceAutomaticTest() {
  if (activeAutomaticTest == AUTO_TEST_NONE) {
    return;
  }

  const uint32_t now = millis();
  uint32_t intervalMs = 0;

  switch (activeAutomaticTest) {
    case AUTO_TEST_RGB_CYCLE:
      intervalMs = RGB_CYCLE_STEP_MS;
      break;

    case AUTO_TEST_RGB_PHASE_ROTATION:
      intervalMs = RGB_PHASE_ROTATION_STEP_MS;
      break;

    case AUTO_TEST_LED_CHASE:
      intervalMs = LED_CHASE_STEP_MS;
      break;

    case AUTO_TEST_BRIGHTNESS_SWEEP:
      // Brief dwell at zero makes the sweep boundary visible.
      if (
        automaticTestStep == 0 &&
        automaticBrightnessDirection == 1
      ) {
        intervalMs = 500;
      }
      else {
        intervalMs = BRIGHTNESS_SWEEP_STEP_MS;
      }
      break;

    case AUTO_TEST_ALL_WHITE:
    case AUTO_TEST_ALL_OFF:
      // Static patterns remain active without repeatedly writing TLC.
      return;

    case AUTO_TEST_NONE:
    default:
      return;
  }

  if (
    now - previousAutomaticTestStepTime <
    intervalMs
  ) {
    return;
  }

  previousAutomaticTestStepTime = now;

  switch (activeAutomaticTest) {
    case AUTO_TEST_RGB_CYCLE:
      automaticTestStep =
        (automaticTestStep + 1U) % 7U;
      break;

    case AUTO_TEST_RGB_PHASE_ROTATION:
      automaticTestStep =
        (automaticTestStep + 1U) % 3U;
      break;

    case AUTO_TEST_LED_CHASE:
      automaticTestStep =
        (automaticTestStep + 1U) % 9U;
      break;

    case AUTO_TEST_BRIGHTNESS_SWEEP: {
      int16_t nextStep =
    static_cast<int16_t>(automaticTestStep) +
    automaticBrightnessDirection;

      if (nextStep >= 255) {
    nextStep = 255;
    automaticBrightnessDirection = -1;
  }
      else if (nextStep <= 0) {
    nextStep = 0;
    automaticBrightnessDirection = 1;
  }

      automaticTestStep =
        static_cast<uint8_t>(nextStep);
      break;
    }
    case AUTO_TEST_ALL_WHITE:
    case AUTO_TEST_ALL_OFF:
    case AUTO_TEST_NONE:
    default:
      return;
  }

  renderAutomaticTestStep();
}

void runSelectedAutomaticTest() {
  if (selectedOption >= TEST_OPTION_COUNT) {
    return;
  }

  const AutomaticTest requestedTest =
    static_cast<AutomaticTest>(
      selectedOption + 1U
    );

  // Selecting the active test again stops it and restores manual GS values.
  if (activeAutomaticTest == requestedTest) {
    stopAutomaticTest(true);
    return;
  }

  startAutomaticTest(requestedTest);
}


void drawDiagnosticGridCell(
  uint8_t outputIndex,
  int16_t x,
  int16_t y
) {
  uint16_t backgroundColor = TFT_DARKGREY;
  uint16_t textColor = TFT_WHITE;

  if (TLC_OUTPUT_CONNECTED[outputIndex]) {
    switch (allOutputStatus[outputIndex]) {
      case LED_STATUS_OK:
        backgroundColor = TFT_GREEN;
        textColor = TFT_BLACK;
        break;

      case LED_STATUS_OPEN:
        backgroundColor = TFT_RED;
        textColor = TFT_WHITE;
        break;

      case LED_STATUS_SHORT:
        backgroundColor = TFT_ORANGE;
        textColor = TFT_BLACK;
        break;

      case LED_STATUS_OUTPUT_SHORT_GND:
        backgroundColor = TFT_MAGENTA;
        textColor = TFT_WHITE;
        break;

      case LED_STATUS_PWM_OR_UNKNOWN:
      default:
        backgroundColor = TFT_YELLOW;
        textColor = TFT_BLACK;
        break;
    }
  }

  constexpr int16_t CELL_WIDTH = 25;
  constexpr int16_t CELL_HEIGHT = 20;

  tft.fillRect(
    x,
    y,
    CELL_WIDTH,
    CELL_HEIGHT,
    backgroundColor
  );

  tft.drawRect(
    x,
    y,
    CELL_WIDTH,
    CELL_HEIGHT,
    TFT_BLACK
  );

  tft.setTextSize(1);
  tft.setTextColor(textColor, backgroundColor);
  tft.setCursor(x + 6, y + 7);
  tft.print(TLC_OUTPUT_NAMES[outputIndex]);
}
