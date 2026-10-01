uint8_t getOptionCount(UiPage page) {
  switch (page) {
    case PAGE_MANUAL:
      return MANUAL_OPTION_COUNT;
    case PAGE_TESTS:
      return TEST_OPTION_COUNT;
    case PAGE_DIAGNOSTICS:
      return DIAGNOSTIC_OPTION_COUNT;
    case PAGE_OVERVIEW:
    default:
      return 0;
  }
}

const char* getPageTitle(UiPage page) {
  switch (page) {
    case PAGE_OVERVIEW:
      return "OVERVIEW";
    case PAGE_MANUAL:
      return "MANUAL CONTROL";
    case PAGE_TESTS:
      return "AUTOMATIC TESTS";
    case PAGE_DIAGNOSTICS:
      return "DIAGNOSTICS";
    default:
      return "UNKNOWN";
  }
}

void drawPageHeader() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(5, 4);
  tft.print(getPageTitle(currentPage));

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(136, 4);
  tft.print(static_cast<int>(currentPage) + 1);
  tft.print("/");
  tft.print(static_cast<int>(PAGE_COUNT));

  tft.drawFastHLine(4, 16, 152, TFT_DARKGREY);
}

void drawPageFooter() {
  tft.fillRect(0, 108, 160, 20, TFT_BLACK);
  tft.drawFastHLine(4, 108, 152, TFT_DARKGREY);

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(4, 113);

  if (getOptionCount(currentPage) > 0) {
    tft.print("P:PAGE O:OPT S:SET");
  } else {
    tft.print("P: NEXT SCREEN");
  }
}

void drawMenuRow(
  int16_t y,
  uint8_t optionIndex,
  const char* label,
  const char* value
) {
  const bool selected = (selectedOption == optionIndex);

  const uint16_t backgroundColor =
    selected ? TFT_DARKGREY : TFT_BLACK;

  const uint16_t textColor =
    selected ? TFT_YELLOW : TFT_WHITE;

  // Clear only this menu row. This removes the previous highlight
  // without erasing or refreshing the complete TFT page.
  tft.fillRect(
    3,
    y - 2,
    154,
    13,
    backgroundColor
  );

  tft.setTextSize(1);
  tft.setTextColor(textColor, backgroundColor);

  tft.setCursor(6, y);
  tft.print(selected ? "> " : "  ");
  tft.print(label);

  if (value != nullptr) {
    tft.setCursor(104, y);
    tft.print(value);
  }
}


void drawOverviewPage() {
  drawPageHeader();

  // LED 0
  tft.drawRect(4, 23, 48, 46, TFT_RED);
  tft.fillCircle(28, 37, 8, TFT_RED);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(13, 51);
  tft.print("LED 0");

  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(18, 61);
  tft.print("RED");

  // LED 1
  tft.drawRect(56, 23, 48, 46, TFT_GREEN);
  tft.fillCircle(80, 37, 8, TFT_GREEN);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(65, 51);
  tft.print("LED 1");

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(63, 61);
  tft.print("GREEN");

  // LED 2
  tft.drawRect(108, 23, 48, 46, TFT_BLUE);
  tft.fillCircle(132, 37, 8, TFT_BLUE);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(117, 51);
  tft.print("LED 2");

  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.setCursor(120, 61);
  tft.print("BLUE");

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(5, 77);
  tft.print("GCLK: 1MHz   IREF: 9.1k");

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(5, 90);
  tft.print("9 / 24 channels wired");

  drawPageFooter();
}

void updateManualValueField(
  int16_t y,
  uint8_t optionIndex,
  const char* value
) {
  const bool selected = (selectedOption == optionIndex);
  const uint16_t background =
    selected ? TFT_DARKGREY : TFT_BLACK;
  const uint16_t textColor =
    selected ? TFT_YELLOW : TFT_WHITE;

  // Clear only the value column of this row.
  tft.fillRect(103, y - 2, 54, 13, background);

  tft.setTextSize(1);
  tft.setTextColor(textColor, background);
  tft.setCursor(104, y);
  tft.print(value);
}

void updateManualDynamicValues() {
  if (currentPage != PAGE_MANUAL) {
    return;
  }

  char gsText[6];
  snprintf(
    gsText,
    sizeof(gsText),
    "%u",
    static_cast<unsigned int>(gsValues[selectedChannel])
  );

  char dcText[4];
  snprintf(
    dcText,
    sizeof(dcText),
    "%u",
    static_cast<unsigned int>(dcValues[selectedChannel])
  );

  const uint8_t bcGroup = getSelectedBcGroup();

  char bcText[8];
  snprintf(
    bcText,
    sizeof(bcText),
    "%s:%u",
    getSelectedBcGroupName(),
    static_cast<unsigned int>(bcValues[bcGroup])
  );

  updateManualValueField(
    24,
    0,
    getSelectedChannelName()
  );

  updateManualValueField(
    40,
    1,
    gsText
  );

  updateManualValueField(
    56,
    2,
    dcText
  );

  updateManualValueField(
    72,
    3,
    bcText
  );

  updateManualValueField(
    88,
    4,
    channelEnabled[selectedChannel] ? "YES" : "NO"
  );
}

void drawManualOption(uint8_t optionIndex) {
  char gsText[6];
  snprintf(
    gsText,
    sizeof(gsText),
    "%u",
    static_cast<unsigned int>(gsValues[selectedChannel])
  );

  char dcText[4];
  snprintf(
    dcText,
    sizeof(dcText),
    "%u",
    static_cast<unsigned int>(dcValues[selectedChannel])
  );

  const uint8_t bcGroup = getSelectedBcGroup();

  char bcText[8];
  snprintf(
    bcText,
    sizeof(bcText),
    "%s:%u",
    getSelectedBcGroupName(),
    static_cast<unsigned int>(bcValues[bcGroup])
  );

  switch (optionIndex) {
    case 0:
      drawMenuRow(
        24,
        0,
        "CHANNEL",
        getSelectedChannelName()
      );
      break;

    case 1:
      drawMenuRow(40, 1, "GS", gsText);
      break;

    case 2:
      drawMenuRow(56, 2, "DC", dcText);
      break;

    case 3:
      drawMenuRow(72, 3, "BC", bcText);
      break;

    case 4:
      drawMenuRow(
        88,
        4,
        "ENABLE",
        channelEnabled[selectedChannel] ? "YES" : "NO"
      );
      break;

    default:
      break;
  }
}

void drawTestsOption(uint8_t optionIndex) {
  switch (optionIndex) {
    case 0:
      drawMenuRow(21, 0, "RGB colour cycle", nullptr);
      break;

    case 1:
      drawMenuRow(34, 1, "RGB phase rotation", nullptr);
      break;

    case 2:
      drawMenuRow(47, 2, "LED chase", nullptr);
      break;

    case 3:
      drawMenuRow(60, 3, "Brightness sweep", nullptr);
      break;

    case 4:
      drawMenuRow(73, 4, "All white", nullptr);
      break;

    case 5:
      drawMenuRow(86, 5, "All off", nullptr);
      break;

    default:
      break;
  }
}

void drawDiagnosticsOption(uint8_t optionIndex) {
  switch (optionIndex) {
    case 0:
      drawMenuRow(32, 0, "NEG/GCLK test", nullptr);
      break;

    case 1:
      drawMenuRow(45, 1, "Clear errors", nullptr);
      break;

    case 2:
      drawMenuRow(58, 2, "Adjacent pin test", nullptr);
      break;

    case 3:
      drawMenuRow(71, 3, "Scan all channels", nullptr);
      break;

    case 4:
      drawMenuRow(84, 4, "Device status", nullptr);
      break;

    case 5:
      drawMenuRow(97, 5, "LOD/LSD self-test", nullptr);
      break;

    default:
      break;
  }
}

void redrawCurrentPageOption(uint8_t optionIndex) {
  if (
    currentPage == PAGE_DIAGNOSTICS &&
    diagnosticsGridVisible
  ) {
    return;
  }

  switch (currentPage) {
    case PAGE_MANUAL:
      drawManualOption(optionIndex);
      break;

    case PAGE_DIAGNOSTICS:
      drawDiagnosticsOption(optionIndex);
      break;

    case PAGE_TESTS:
      drawTestsOption(optionIndex);
      break;

    case PAGE_OVERVIEW:
    default:
      break;
  }
}

void drawManualPage() {
  drawPageHeader();

  char gsText[6];
  snprintf(
    gsText,
    sizeof(gsText),
    "%u",
    static_cast<unsigned int>(gsValues[selectedChannel])
  );

  char dcText[4];
  snprintf(
    dcText,
    sizeof(dcText),
    "%u",
    static_cast<unsigned int>(dcValues[selectedChannel])
  );

  const uint8_t bcGroup = getSelectedBcGroup();

  char bcText[8];
  snprintf(
    bcText,
    sizeof(bcText),
    "%s:%u",
    getSelectedBcGroupName(),
    static_cast<unsigned int>(bcValues[bcGroup])
  );

  drawMenuRow(
    24,
    0,
    "CHANNEL",
    getSelectedChannelName()
  );

  drawMenuRow(
    40,
    1,
    "GS",
    gsText
  );

  drawMenuRow(56, 2, "DC", dcText);
  drawMenuRow(72, 3, "BC", bcText);

  drawMenuRow(
    88,
    4,
    "ENABLE",
    channelEnabled[selectedChannel] ? "YES" : "NO"
  );

  drawPageFooter();
}

void drawTestsPage() {
  drawPageHeader();

  drawMenuRow(21, 0, "RGB colour cycle", nullptr);
  drawMenuRow(34, 1, "RGB phase rotation", nullptr);
  drawMenuRow(47, 2, "LED chase", nullptr);
  drawMenuRow(60, 3, "Brightness sweep", nullptr);
  drawMenuRow(73, 4, "All white", nullptr);
  drawMenuRow(86, 5, "All off", nullptr);

  drawPageFooter();

  if (activeAutomaticTest != AUTO_TEST_NONE) {
    char message[32];

    snprintf(
      message,
      sizeof(message),
      "RUN: %s",
      getAutomaticTestLabel(activeAutomaticTest)
    );

    showAutomaticTestMessage(
      message,
      TFT_CYAN
    );
  }
}


void drawCurrentPage() {
  switch (currentPage) {
    case PAGE_OVERVIEW:
      drawOverviewPage();
      break;

    case PAGE_MANUAL:
      drawManualPage();
      break;

    case PAGE_TESTS:
      drawTestsPage();
      break;

    case PAGE_DIAGNOSTICS:
      drawDiagnosticsPage();
      break;

    default:
      currentPage = PAGE_OVERVIEW;
      selectedOption = 0;
      drawOverviewPage();
      break;
  }
}

void cycleSelectedManualValue() {
  if (currentPage != PAGE_MANUAL) {
    return;
  }

  switch (selectedOption) {
    case 0:
      selectedChannel = (selectedChannel + 1) % CHANNEL_COUNT;
      updateManualDynamicValues();
      Serial.print("Selected channel: ");
      Serial.println(getSelectedChannelName());
      break;

    case 1: {
      size_t levelIndex = findClosestGsLevel(gsValues[selectedChannel]);
      levelIndex = (levelIndex + 1) % GS_LEVEL_COUNT;
      gsValues[selectedChannel] = GS_LEVELS[levelIndex];
      applyGsValues();
      updateManualDynamicValues();
      Serial.print(getSelectedChannelName());
      Serial.print(" GS = 0x");
      Serial.print(gsValues[selectedChannel], HEX);
      Serial.print(" / ");
      Serial.println(gsValues[selectedChannel]);
      break;
    }

    case 2: {
      size_t levelIndex = findClosestDcLevel(dcValues[selectedChannel]);
      levelIndex = (levelIndex + 1) % DC_LEVEL_COUNT;
      dcValues[selectedChannel] = DC_LEVELS[levelIndex];
      applyFcValues();
      updateManualDynamicValues();
      Serial.print(getSelectedChannelName());
      Serial.print(" DC = ");
      Serial.println(dcValues[selectedChannel]);
      break;
    }

    case 3: {
      const uint8_t bcGroup = getSelectedBcGroup();
      size_t levelIndex = findClosestBcLevel(bcValues[bcGroup]);
      levelIndex = (levelIndex + 1) % BC_LEVEL_COUNT;
      bcValues[bcGroup] = BC_LEVELS[levelIndex];
      applyFcValues();
      updateManualDynamicValues();
      Serial.print("BC_");
      Serial.print(getSelectedBcGroupName());
      Serial.print(" = ");
      Serial.println(bcValues[bcGroup]);
      break;
    }

    case 4:
      channelEnabled[selectedChannel] = !channelEnabled[selectedChannel];
      applyGsValues();
      updateManualDynamicValues();
      Serial.print(getSelectedChannelName());
      Serial.print(" enabled: ");
      Serial.println(channelEnabled[selectedChannel] ? "YES" : "NO");
      break;

    default:
      break;
  }
}


void changePage(int8_t direction) {
  // Leaving the scan grid returns Diagnostics to its normal menu
  // the next time that page is opened.
  diagnosticsGridVisible = false;

  /*
   * A local PAGE press while an automatic demonstration is active
   * is also the local escape/stop action. Restore the user's normal
   * LED pattern before opening the next page.
   */
  if (
    currentPage == PAGE_TESTS &&
    activeAutomaticTest != AUTO_TEST_NONE
  ) {
    stopAutomaticTest(true);
  }

  int8_t newPage = static_cast<int8_t>(currentPage) + direction;

  if (newPage < 0) {
    newPage = PAGE_COUNT - 1;
  } else if (newPage >= PAGE_COUNT) {
    newPage = 0;
  }

  currentPage = static_cast<UiPage>(newPage);
  selectedOption = 0;

  drawCurrentPage();

  Serial.print("Page: ");
  Serial.println(getPageTitle(currentPage));
}

void changeOption(int8_t direction) {
  // There are no selectable rows on the result grid.
  if (
    currentPage == PAGE_DIAGNOSTICS &&
    diagnosticsGridVisible
  ) {
    return;
  }

  const uint8_t optionCount = getOptionCount(currentPage);

  if (optionCount == 0) {
    return;
  }

  const uint8_t previousOption = selectedOption;

  int8_t newOption =
    static_cast<int8_t>(selectedOption) + direction;

  if (newOption < 0) {
    newOption = optionCount - 1;
  } else if (newOption >= optionCount) {
    newOption = 0;
  }

  selectedOption = static_cast<uint8_t>(newOption);

  // Refresh only two rows:
  // 1. the old row, to remove its highlight
  // 2. the new row, to add the highlight
  redrawCurrentPageOption(previousOption);
  redrawCurrentPageOption(selectedOption);

  Serial.print("Selected option: ");
  Serial.println(selectedOption + 1);
}

void activateSelectedOption() {
  if (getOptionCount(currentPage) == 0) {
    Serial.println("No selectable item on this page.");
    return;
  }

  if (currentPage == PAGE_MANUAL) {
    cycleSelectedManualValue();
    return;
  }

  if (currentPage == PAGE_DIAGNOSTICS) {
    if (diagnosticsGridVisible) {
      captureAllOutputSid();
      printAllOutputSid();
      drawDiagnosticsGrid();
    } else {
      runSelectedDiagnostic();
    }

    return;
  }

  if (currentPage == PAGE_TESTS) {
    runSelectedAutomaticTest();
    return;
  }
}
