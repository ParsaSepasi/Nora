#include <FastLED.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

#if DEBUG_SERIAL
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

// ------------------- GPIO -------------------
void GPIO(int GPIOIndex, bool state) {
  if (GPIOIndex >= 0 && GPIOIndex < NUM_PINS) {
    int pin = GPIOPins[GPIOIndex];
    digitalWrite(pin, state ? HIGH : LOW);
    if (state) {
      Serial.print(F("ON: "));
      Serial.print(GPIONames[GPIOIndex]);
      Serial.print(F(" on GPIO "));
      Serial.println(pin);
    } else {
      Serial.print(F("OFF: "));
      Serial.print(GPIONames[GPIOIndex]);
      Serial.print(F(" on GPIO "));
      Serial.println(pin);
    }
  } else {
    Serial.print(F("Invalid GPIO index: "));
    Serial.println(GPIOIndex);
  }
}

// ------------------- Serial Commands -------------------
void handleSerialCommand(String command) {
  command.toUpperCase();  // Convert to upper case once
  command.trim();         // Remove extra spaces

  String noSpace = command;
  noSpace.replace(" ", "");  // For commands without spaces

  if (command == "EQUALIZER 1") {
    equalizer1Active = true;
    equalizer2Active = false;
    equalizer3Active = false;
    Serial.println(F("🎛️ EQ1 Started"));
  } else if (command == "EQUALIZER 2") {
    equalizer1Active = false;
    equalizer2Active = true;
    equalizer3Active = false;
    Serial.println(F("🎛️ EQ2 Started"));
  } else if (command == "EQUALIZER 3") {
    equalizer1Active = false;
    equalizer2Active = false;
    equalizer3Active = true;
    smoothedLevel = 0;
    dynamicMin = 4095;
    dynamicMax = 0;
    dynamicRangeValid = false;
    lastCalibrate = millis();
    Serial.println(F("🎛️ EQ3 Started"));
  }else if (command == "BOX EQUALIZER 1") {
    boxequalizer1Active = true;
    boxequalizer2Active = false;
    boxequalizer3Active = false;
    Serial.println(F("🎛️BOX EQ1 Started"));
  } else if (command == "BOX EQUALIZER 2") {
    boxequalizer1Active = false;
    boxequalizer2Active = true;
    boxequalizer3Active = false;
    Serial.println(F("🎛️BOX EQ2 Started"));
  } else if (command == "BOX EQUALIZER 3") {
    boxequalizer1Active = false;
    boxequalizer2Active = false;
    boxequalizer3Active = true;
    smoothedLevel = 0;
    dynamicMin = 4095;
    dynamicMax = 0;
    dynamicRangeValid = false;
    lastCalibrate = millis();
    Serial.println(F("🎛️BOX EQ3 Started"));
    }else if (command == "EQUALIZER OFF") {
    equalizer1Active = false;
    equalizer2Active = false;
    equalizer3Active = false;
    FastLED.clear(true);
    Serial.println(F("🛑 EQ OFF"));
  }else if (command == "BOX EQUALIZER OFF") {
    boxequalizer1Active = false;
    boxequalizer2Active = false;
    boxequalizer3Active = false;
    FastLED.clear(true);
    Serial.println(F("🛑BOX EQ OFF"));
  }else if (command == "WAKE WORD") {
    run_led_wake_word();
    Serial.println(F("🎛️ Wake word Started"));
  } else {
    // Check GPIO
    for (int i = 0; i < NUM_PINS; i++) {
      String onCmd = GPIONames[i] + "_ON";
      String offCmd = GPIONames[i] + "_OFF";
      if (command == onCmd) {
        GPIO(i, true);
        return;
      } else if (command == offCmd) {
        GPIO(i, false);
        return;
      }
    }
  }

  // RGB
  if (command.startsWith("RGB:")) {
    parseRGBCommand(command.substring(4));
  }

  //Clock
  if (command.startsWith("TIME:")){
    String timeString = command.substring(5);
    timeString.trim();

    if (timeString.length() == 8 && timeString[2] == ':' && timeString[5] == ':'){
      currentTime = timeString;
      Serial.print(F("🕒 Time set to: "));
      Serial.println(currentTime);
    } else {
    Serial.print(F("❌ Invalid time format: "));
    Serial.println(timeString);
    }
    return;
  }

  // Modes
  if (noSpace == "SOUND_OFF") {
    sound_system_off();
    return;
  }
  if (noSpace == "NORMAL") {
    close_box();
    return;
  }
  if (noSpace == "PARTY") {
    open_box();
    return;
  }

  // Unknown command
  Serial.print(F("❌ Unknown command: "));
  Serial.println(command);
}

// ------------------- Equalizer Functions -------------------
void runEqualizer1() {
  // Simple wave pattern: LEDs light up sequentially based on sound level
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.1 * raw) + (0.9 * smoothedLevel); // Smoother transition
  int litLeds = map(smoothedLevel, 0, 4095, 0, NUM_LEDS);
  litLeds = constrain(litLeds, 0, NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < litLeds) {
      uint8_t hue = colorIndex + i * 10; // Wider color spread
      leds[i] = ColorFromPalette(currentPalette, hue, 255, currentBlending);
    } else {
      leds[i] = CRGB::Black;
    }
  }
  colorIndex += 2; // Faster color cycling
  FastLED.show();
}

void runEqualizer2() {
  // Pulse effect: All LEDs pulse with same color based on sound level
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel); // Very smooth
  uint8_t brightness = map(smoothedLevel, 0, 4095, 0, 255);
  brightness = constrain(brightness, 0, 255);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
  }
  colorIndex++; // Slow color cycling
  FastLED.show();
}

void runEqualizer3() {
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel);
  if (smoothedLevel < dynamicMin) dynamicMin = smoothedLevel;
  if (smoothedLevel > dynamicMax) dynamicMax = smoothedLevel;
  if (millis() - lastCalibrate >= 3000) {
    int range = dynamicMax - dynamicMin;
    dynamicRangeValid = (range >= 25);
    dynamicMin = 4095;
    dynamicMax = 0;
    lastCalibrate = millis();
  }
  int litLeds = 0;
  if (dynamicRangeValid) {
    litLeds = map(smoothedLevel, dynamicMin, dynamicMax, 0, NUM_LEDS / 2);
    litLeds = constrain(litLeds, 0, NUM_LEDS / 2);
  }
  int mid = NUM_LEDS / 2;
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
  for (int i = 0; i < litLeds; i++) {
    uint8_t hue = colorIndex + i * 6;
    leds[mid - 1 - i] = ColorFromPalette(RainbowColors_p, hue);
    leds[mid + i] = ColorFromPalette(RainbowColors_p, hue);
  }
  colorIndex++;
  FastLED.show();
}

void runBOXEqualizer1() {
  // Simple wave pattern: LEDs light up sequentially based on sound level
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.1 * raw) + (0.9 * smoothedLevel); // Smoother transition
  int litLeds = map(smoothedLevel, 0, 4095, 0, NUM_LEDS);
  litLeds = constrain(litLeds, 0, NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < litLeds) {
      uint8_t hue = colorIndex + i * 10; // Wider color spread
      leds[i] = ColorFromPalette(currentPalette, hue, 255, currentBlending);
    } else {
      leds[i] = CRGB::Black;
    }
  }
  colorIndex += 2; // Faster color cycling
  FastLED.show();
}

void runBOXEqualizer2() {
  // Pulse effect: All LEDs pulse with same color based on sound level
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel); // Very smooth
  uint8_t brightness = map(smoothedLevel, 0, 4095, 0, 255);
  brightness = constrain(brightness, 0, 255);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
  }
  colorIndex++; // Slow color cycling
  FastLED.show();
}

void runBOXEqualizer3() {
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel);
  if (smoothedLevel < dynamicMin) dynamicMin = smoothedLevel;
  if (smoothedLevel > dynamicMax) dynamicMax = smoothedLevel;
  if (millis() - lastCalibrate >= 3000) {
    int range = dynamicMax - dynamicMin;
    dynamicRangeValid = (range >= 25);
    dynamicMin = 4095;
    dynamicMax = 0;
    lastCalibrate = millis();
  }
  int litLeds = 0;
  if (dynamicRangeValid) {
    litLeds = map(smoothedLevel, dynamicMin, dynamicMax, 0, NUM_LEDS / 2);
    litLeds = constrain(litLeds, 0, NUM_LEDS / 2);
  }
  int mid = NUM_LEDS / 2;
  for (int i = 0; i < NUM_LEDS; i++) leds[i] = CRGB::Black;
  for (int i = 0; i < litLeds; i++) {
    uint8_t hue = colorIndex + i * 6;
    leds[mid - 1 - i] = ColorFromPalette(RainbowColors_p, hue);
    leds[mid + i] = ColorFromPalette(RainbowColors_p, hue);
  }
  colorIndex++;
  FastLED.show();
}

// ------------------- Modes -------------------
void sound_system_off() {
  digitalWrite(MUTE, HIGH);
  digitalWrite(PARTY, LOW);
  Serial.println(F("🔇 Sound System OFF (Mute + PartyOff)"));
}

void normal_mode() {
  digitalWrite(MUTE, LOW);
  digitalWrite(PARTY, LOW);
  Serial.println(F("🎵 Normal Mode (Unmute + PartyOff)"));
}

void party_mode() {
  digitalWrite(MUTE, LOW);
  digitalWrite(PARTY, HIGH);
  Serial.println(F("🎉 Party Mode (Unmute + PartyOn)"));
}

// ------------------- Box -------------------
void open_box() {
  digitalWrite(OPEN_BOX, HIGH);
  relayOnTime = millis();
  relayActive = true;
  Serial.println(F("box opened"));
}

void close_box() {
  digitalWrite(CLOSE_BOX, HIGH);
  relayOnTime = millis();
  relayActive = true;
  Serial.println(F("box closed"));
}

// ------------------- Wake word -------------------
void run_led_wake_word() {
  const int sections = 5;
  int ledsPerSection = NUM_LEDS / sections;
  CRGB wakeColor = CRGB(0, 255, 255);
  Serial.println(F("Wake word says hello!"));
  fill_solid(leds, NUM_LEDS, wakeColor);
  FastLED.show();
  for (int sec = 0; sec < sections; sec++) {
    delay(1000);
    int start = sec * ledsPerSection;
    int end = (sec == sections - 1) ? NUM_LEDS : start + ledsPerSection;
    for (int i = start; i < end; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  }
}

// TODO: Implement parseRGBCommand if needed
void parseRGBCommand(String rgbString) {
  Serial.print(F("RGB command parsed: "));
  Serial.println(rgbString);
}

