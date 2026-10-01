void clearFrame() {
  memset(frame, 0, sizeof(frame));
}

// TLC bit 287 -> frame[0], bit 7
// TLC bit   0 -> frame[35], bit 0
void setBitTLC(uint16_t bit, bool value) {
  if (bit > 287) {
    return;
  }

  const uint16_t serialPosition = 287 - bit;
  const uint8_t byteIndex = serialPosition / 8;
  const uint8_t bitIndex = 7 - (serialPosition % 8);

  if (value) {
    frame[byteIndex] |= (1U << bitIndex);
  } else {
    frame[byteIndex] &= ~(1U << bitIndex);
  }
}

bool getBitTLC(uint16_t bit) {
  if (bit > 287) {
    return false;
  }

  const uint16_t serialPosition = 287 - bit;
  const uint8_t byteIndex = serialPosition / 8;
  const uint8_t bitIndex = 7 - (serialPosition % 8);

  return (frame[byteIndex] >> bitIndex) & 0x01;
}

void setFieldTLC(uint16_t msb, uint16_t lsb, uint32_t value) {
  if (msb < lsb || msb > 287) {
    return;
  }

  for (uint16_t i = 0; i <= (msb - lsb); i++) {
    setBitTLC(lsb + i, (value >> i) & 0x01);
  }
}

void clearSidFrame() {
  memset(sidFrame, 0, sizeof(sidFrame));
}

void setBufferBit(
  uint8_t* buffer,
  uint16_t bit,
  bool value
) {
  if (buffer == nullptr || bit > 287) {
    return;
  }

  const uint16_t serialPosition = 287 - bit;
  const uint8_t byteIndex = serialPosition / 8;
  const uint8_t bitIndex = 7 - (serialPosition % 8);

  if (value) {
    buffer[byteIndex] |= (1U << bitIndex);
  } else {
    buffer[byteIndex] &= ~(1U << bitIndex);
  }
}

bool getBufferBit(
  const uint8_t* buffer,
  uint16_t bit
) {
  if (buffer == nullptr || bit > 287) {
    return false;
  }

  const uint16_t serialPosition = 287 - bit;
  const uint8_t byteIndex = serialPosition / 8;
  const uint8_t bitIndex = 7 - (serialPosition % 8);

  return (buffer[byteIndex] >> bitIndex) & 0x01;
}


void pulseSCK() {
  digitalWrite(PIN_SCK, HIGH);
  delayMicroseconds(10);

  digitalWrite(PIN_SCK, LOW);
  delayMicroseconds(10);
}

void pulseSCKFast() {
  digitalWrite(PIN_SCK, HIGH);
  delayMicroseconds(1);

  digitalWrite(PIN_SCK, LOW);
  delayMicroseconds(1);
}
void sendFCFrame() {
  digitalWrite(PIN_BLANK, LOW);
  digitalWrite(PIN_SCK, LOW);
  digitalWrite(PIN_MOSI, LOW);

  // LATCH is normally already HIGH because sendGSFrame()
  // now leaves it in the safe idle state. Therefore this line
  // does not create an unintended rising edge before shifting.
  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(20);

  for (int bit = 287; bit >= 0; bit--) {
    digitalWrite(PIN_MOSI, getBitTLC(bit) ? HIGH : LOW);
    delayMicroseconds(10);
    pulseSCK();
  }

  delayMicroseconds(20);

  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(20);

  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(20);

  digitalWrite(PIN_MOSI, LOW);
  delayMicroseconds(100);
}

void sendFCFrameFast() {
  digitalWrite(PIN_BLANK, LOW);
  digitalWrite(PIN_SCK, LOW);
  digitalWrite(PIN_MOSI, LOW);

  // FC/BC/DC frame requires LATCH HIGH on the 288th SCK edge.
  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(1);

  for (int bit = 287; bit >= 0; bit--) {
    digitalWrite(
      PIN_MOSI,
      getBitTLC(bit) ? HIGH : LOW
    );

    delayMicroseconds(1);
    pulseSCKFast();
  }

  delayMicroseconds(1);

  // Transfer completed FC/BC/DC frame.
  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(1);

  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(1);

  digitalWrite(PIN_MOSI, LOW);
}

void sendGSFrame() {
  digitalWrite(PIN_BLANK, LOW);
  digitalWrite(PIN_SCK, LOW);

  // LATCH must be LOW at the 288th SCK rising edge so the
  // 288-bit common register is interpreted as GS data.
  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(20);

  for (int bit = 287; bit >= 0; bit--) {
    digitalWrite(PIN_MOSI, getBitTLC(bit) ? HIGH : LOW);
    delayMicroseconds(10);
    pulseSCK();
  }

  delayMicroseconds(20);

  // Rising edge transfers the completed frame into the GS latch.
  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(100);

  // IMPORTANT:
  // Leave LATCH HIGH as the idle state.
  //
  // After a GS write, the TLC loads FC/BC/DC data back into the
  // common shift register. If we ended this function with LATCH LOW,
  // the next sendFCFrame() would begin by raising LATCH and could
  // unintentionally latch that common-register content into GS again.
}

void sendGSFrameFast() {
  digitalWrite(PIN_BLANK, LOW);
  digitalWrite(PIN_SCK, LOW);

  // LOW during the 288th SCK rising edge = GS frame.
  digitalWrite(PIN_LATCH, LOW);
  delayMicroseconds(1);

  for (int bit = 287; bit >= 0; bit--) {
    digitalWrite(PIN_MOSI, getBitTLC(bit) ? HIGH : LOW);

    delayMicroseconds(1);
    pulseSCKFast();
  }

  delayMicroseconds(1);

  // Rising edge transfers the completed frame into GS latch.
  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(1);

  // Return data line to a defined idle state.
  digitalWrite(PIN_MOSI, LOW);

  // Keep LATCH HIGH, same as our validated sendGSFrame().
}
void sendSpecialCommand(uint16_t command) {
  clearFrame();
  setFieldTLC(287, 276, command & 0x0FFF);

  digitalWrite(PIN_BLANK, LOW);
  digitalWrite(PIN_SCK, LOW);
  digitalWrite(PIN_LATCH, LOW);

  for (int bit = 287; bit >= 0; bit--) {
    digitalWrite(PIN_MOSI, getBitTLC(bit) ? HIGH : LOW);
    delayMicroseconds(10);

    if (bit == 0) {
      digitalWrite(PIN_LATCH, HIGH);
      delayMicroseconds(20);
    }

    pulseSCK();

    if (bit == 0) {
      delayMicroseconds(20);
      digitalWrite(PIN_LATCH, LOW);
    }
  }

  delayMicroseconds(100);
}

void readSidIntoBuffer() {
  clearSidFrame();

  // 0x5A3 loads SID into the common 288-bit shift register.
  sendSpecialCommand(0x5A3);

  // The command function finishes LOW. This rising edge ensures
  // that the command is executed and leaves LATCH stable during
  // the following 288 read clocks.
  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(20);

  digitalWrite(PIN_SCK, LOW);
  digitalWrite(PIN_MOSI, LOW);

  // The register MSB, TLC bit 287, is already present on SDO
  // before the first read clock. Sample first, then shift.
  for (int bit = 287; bit >= 0; bit--) {
    delayMicroseconds(2);

    const bool value =
      digitalRead(PIN_MISO) == HIGH;

    setBufferBit(
      sidFrame,
      static_cast<uint16_t>(bit),
      value
    );

    pulseSCK();
  }

  digitalWrite(PIN_LATCH, HIGH);
  delayMicroseconds(20);

  // sendSpecialCommand() drives BLANK LOW.
  // Always re-enable the TLC outputs after the SID frame
  // has been completely shifted into sidFrame.
  digitalWrite(PIN_BLANK, HIGH);
}


void buildFCThreeRGB() {
  clearFrame();

  // Normal FC/BC/DC write
  setFieldTLC(287, 276, 0x000);

  // Function-control configuration
  setBitTLC(204, 1);          // LED_ERR_MASK
  setBitTLC(203, 0);          // Slower output slew rate
  setBitTLC(202, 0);          // LOD threshold selection
  setBitTLC(201, 0);          // LSD threshold selection
  setBitTLC(200, 0);          // APS_CURRENT disabled
  setBitTLC(199, 0);          // APS_TIME disabled

  setFieldTLC(198, 197, 0);   // 12-bit GS mode
  setBitTLC(196, 1);          // TIMING_RESET
  setBitTLC(195, 1);          // AUTO_REPEAT

  // Upper DC range for red, green and blue
  setBitTLC(194, 1);          // Blue
  setBitTLC(193, 1);          // Green
  setBitTLC(192, 1);          // Red

  // Group brightness control:
  // bcValues[0] = Red, bcValues[1] = Green, bcValues[2] = Blue
  setFieldTLC(191, 184, bcValues[2]);  // Blue BC
  setFieldTLC(183, 176, bcValues[1]);  // Green BC
  setFieldTLC(175, 168, bcValues[0]);  // Red BC

  // Per-output dot correction for all 24 TLC outputs.
  for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
    uint8_t value = dcValues[channel];

    if (value > 127) {
      value = 127;
    }

    const uint16_t dcLsb =
      static_cast<uint16_t>(channel) * 7U;

    setFieldTLC(
      dcLsb + 6,
      dcLsb,
      value
    );
  }
}


void buildGSThreeRGB() {
  clearFrame();

  // Every TLC output uses one contiguous 12-bit GS field.
  for (uint8_t channel = 0; channel < CHANNEL_COUNT; channel++) {
    uint16_t value = channelEnabled[channel]
      ? gsValues[channel]
      : 0x000;

    if (value > 0x0FFF) {
      value = 0x0FFF;
    }

    const uint16_t gsLsb =
      static_cast<uint16_t>(channel) * 12U;

    setFieldTLC(
      gsLsb + 11,
      gsLsb,
      value
    );
  }
}

void buildDiagnosticGsSingleOutput(
  uint8_t outputIndex
) {
  clearFrame();

  if (outputIndex >= TLC_OUTPUT_COUNT) {
    return;
  }

  // Test one output at a time.
  // GS = 0x800 keeps this output ON at the early LOD/LSD sample
  // and OFF at the late sample in 12-bit mode.
  const uint16_t lsb =
    static_cast<uint16_t>(outputIndex) * 12U;

  setFieldTLC(
    lsb + 11,
    lsb,
    0x0800
  );
}

void applyGsValues() {
  buildGSThreeRGB();
  sendGSFrame();

  // sendGSFrame() holds BLANK LOW during the transfer.
  // Re-enable the outputs after the new GS frame is latched.
  digitalWrite(PIN_BLANK, HIGH);
}

void applyGsValuesFast() {
  buildGSThreeRGB();

  sendGSFrameFast();

  // Restart the GS counter with the newly latched values.
  digitalWrite(PIN_BLANK, HIGH);
}

void applyFcValues() {
  buildFCThreeRGB();
  sendFCFrame();

  // sendFCFrame() holds BLANK LOW during the transfer.
  // Re-enable the outputs after the FC/DC update.
  digitalWrite(PIN_BLANK, HIGH);
}
void applyFcValuesFast() {
  buildFCThreeRGB();
  sendFCFrameFast();

  digitalWrite(PIN_BLANK, HIGH);
}
void beginDiagnosticCurrentProfile(
  uint8_t savedBc[3]
) {
  for (uint8_t group = 0; group < 3; group++) {
    savedBc[group] = bcValues[group];
    bcValues[group] = DIAGNOSTIC_BC;
  }

  applyFcValues();

  Serial.print("Diagnostic BC profile active: ");
  Serial.println(DIAGNOSTIC_BC);
}

void restoreDiagnosticCurrentProfile(
  const uint8_t savedBc[3]
) {
  for (uint8_t group = 0; group < 3; group++) {
    bcValues[group] = savedBc[group];
  }

  applyFcValues();

  Serial.println("Normal BC values restored.");
}

const char* getSelectedChannelName() {
  return CHANNEL_NAMES[selectedChannel];
}

uint8_t getSelectedBcGroup() {
  return selectedChannel % 3;
}

const char* getSelectedBcGroupName() {
  return BC_GROUP_NAMES[getSelectedBcGroup()];
}

size_t findClosestGsLevel(uint16_t value) {
  size_t closestIndex = 0;
  uint16_t smallestDifference = 0xFFFF;

  for (size_t index = 0; index < GS_LEVEL_COUNT; index++) {
    const uint16_t level = GS_LEVELS[index];
    const uint16_t difference =
      (value >= level) ? (value - level) : (level - value);

    if (difference < smallestDifference) {
      smallestDifference = difference;
      closestIndex = index;
    }
  }

  return closestIndex;
}

size_t findClosestDcLevel(uint8_t value) {
  size_t closestIndex = 0;
  uint8_t smallestDifference = 0xFF;

  for (size_t index = 0; index < DC_LEVEL_COUNT; index++) {
    const uint8_t level = DC_LEVELS[index];
    const uint8_t difference =
      (value >= level) ? (value - level) : (level - value);

    if (difference < smallestDifference) {
      smallestDifference = difference;
      closestIndex = index;
    }
  }

  return closestIndex;
}

size_t findClosestBcLevel(uint8_t value) {
  size_t closestIndex = 0;
  uint8_t smallestDifference = 0xFF;

  for (size_t index = 0; index < BC_LEVEL_COUNT; index++) {
    const uint8_t level = BC_LEVELS[index];
    const uint8_t difference =
      (value >= level) ? (value - level) : (level - value);

    if (difference < smallestDifference) {
      smallestDifference = difference;
      closestIndex = index;
    }
  }

  return closestIndex;
}
