void initializeButtons() {
  for (size_t index = 0; index < BUTTON_COUNT; index++) {
    pinMode(buttons[index].pin, INPUT_PULLUP);

    const uint8_t initialState =
      static_cast<uint8_t>(digitalRead(buttons[index].pin));

    buttons[index].lastRawState = initialState;
    buttons[index].stableState = initialState;
    buttons[index].lastRawChangeTime = millis();
  }
}

UiKey readUiButtons() {
  const uint32_t now = millis();

  for (size_t index = 0; index < BUTTON_COUNT; index++) {
    ButtonState& button = buttons[index];

    const uint8_t rawState =
      static_cast<uint8_t>(digitalRead(button.pin));

    // A raw transition may be switch bounce or a genuine press/release.
    if (rawState != button.lastRawState) {
      button.lastRawState = rawState;
      button.lastRawChangeTime = now;
    }

    // Accept the new state only after it remains stable long enough.
    if (
      now - button.lastRawChangeTime >= BUTTON_DEBOUNCE_MS &&
      rawState != button.stableState
    ) {
      button.stableState = rawState;

      // Generate one UI event only on the HIGH -> LOW press transition.
      if (button.stableState == LOW) {
        return button.key;
      }
    }
  }

  return KEY_NONE;
}

const char* getUiKeyName(UiKey key) {
  switch (key) {
    case KEY_PAGE:
      return "PAGE BUTTON";
    case KEY_OPTION:
      return "OPTION BUTTON";
    case KEY_SELECT:
      return "SELECT BUTTON";
    case KEY_NONE:
    default:
      return "NONE";
  }
}
