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

// CRC-8 function
uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80) crc = (crc << 1) ^ 0x07;
      else crc <<= 1;
    }
  }
  return crc;
}

// Helper function for lowercase conversion
String toLowerCaseString(String str) {
  String result = str;
  for (int i = 0; i < result.length(); i++) {
    if (result[i] >= 'A' && result[i] <= 'Z') {
      result[i] = result[i] + 32;  // Convert to lowercase (ASCII difference)
    }
  }
  return result;
}

// Hex to CRGB conversion
CRGB hexToCRGB(String hexStr) {
  if (hexStr.startsWith("#")) hexStr = hexStr.substring(1);
  if (hexStr.length() == 6) {
    int r = strtol(hexStr.substring(0, 2).c_str(), NULL, 16);
    int g = strtol(hexStr.substring(2, 4).c_str(), NULL, 16);
    int b = strtol(hexStr.substring(4, 6).c_str(), NULL, 16);
    return CRGB(r, g, b);
  }
  return CRGB::Red;  // Default
}

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
#if DEBUG_SERIAL
  Serial.print(F("Processing command: '"));
  Serial.print(command);
  Serial.println(F("'"));
#endif

  command.toUpperCase();
  command.trim();

  // // فقط فرمان‌های جدید با هدر NORA پردازش می‌شوند
  // if (command.startsWith("NORA_")) {
  //   String noHeader = command.substring(5);  // Remove "NORA_"
  //   noHeader.trim();

  // Check CRC if present
  int crcPos = command.lastIndexOf('_');
  if (crcPos > 0 && (command.length() - crcPos - 1) == 2) {  // Check if 2 chars after _
    String crcStr = command.substring(crcPos + 1);
    // Validate crcStr is hex (simple check: only 0-9, A-F)
    bool isValidHex = true;
    for (int i = 0; i < crcStr.length(); i++) {
      char c = crcStr[i];
      if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
        isValidHex = false;
        break;
      }
    }
    if (isValidHex) {
      String payload = command.substring(0, crcPos);
      uint8_t expectedCrc = strtol(crcStr.c_str(), NULL, 16);
      uint8_t calcCrc = crc8((uint8_t *)payload.c_str(), payload.length());
      if (calcCrc != expectedCrc) {
        Serial.println(F("❌ CRC Mismatch!"));
        return;
      }
      Serial.println(F("✅ CRC OK"));
      command = payload;  // Proceed with payload
    }
  }

  // Split by '_' into parts
  String parts[10];  // Max 10 parts
  int partCount = 0;
  int lastIndex = 0;
  for (int i = 0; i < command.length() && partCount < 10; i++) {
    if (command[i] == '_') {
      parts[partCount] = command.substring(lastIndex, i);
      parts[partCount].trim();
      lastIndex = i + 1;
      partCount++;
    }
  }
  // Last part
  if (lastIndex < command.length()) {
    parts[partCount] = command.substring(lastIndex);
    parts[partCount].trim();
    partCount++;
  }

#if DEBUG_SERIAL
  Serial.print(F("Parsed parts: "));
  for (int i = 0; i < partCount; i++) {
    Serial.print(F("'"));
    Serial.print(parts[i]);
    Serial.print(F("' "));
  }
  Serial.println();
#endif

  // Process parts hierarchically
  if (partCount >= 2) {
    String component = toLowerCaseString(parts[0]);
    if (component == "magicl" || component == "magicbl") {
      ledComponent = component;
      if (partCount >= 3 && parts[1] == "MODE") {
        ledMode = toLowerCaseString(parts[2]);  // Use custom toLowerCaseString
        if (ledMode == "off") {
          FastLED.clear(true);
          Serial.println(F("🛑 LED Off"));
        } else if (ledMode == "equalize") {
          if (partCount >= 4 && parts[3].toInt() > 0) {
            equalizer1Active = true;
            Serial.print(F("🎛️ Equalizer Mode Level: "));
            Serial.println(parts[3]);
          } else {
            equalizer1Active = true;
            Serial.println(F("🎛️ Equalizer Mode (Level 1)"));
          }
        } else if (ledMode == "wakeup") {
          run_led_wake_word();
          Serial.println(F("🌅 Wakeup Mode"));
        } else if (ledMode == "rainbow" || ledMode == "static") {
          if (ledMode == "rainbow") {
            currentPalette = RainbowColors_p;
          } else {
            if (partCount >= 4 && parts[3].startsWith("#")) {
              ledColor = parts[3];
              parseRGBCommand(ledColor);
            }
          }
          Serial.print(F("🌈 LED Mode: "));
          Serial.print(ledMode);
          Serial.print(F(" Color: "));
          Serial.println(ledColor);
        }
      }
      if (partCount >= 3 && parts[1] == "BRIGHTNESS") {
        if (parts[2] == "LOW") brightnessLevel = 0;
        else if (parts[2] == "MID") brightnessLevel = 1;
        else if (parts[2] == "HIGH") brightnessLevel = 2;
        FastLED.setBrightness(brightnessLevel * 85 + 50);
        Serial.print(F("💡 Brightness: "));
        Serial.println(parts[2]);
      }
    } else if (component == "clock") {
      if (partCount >= 3 && parts[1] == "TIME") {
        clockTime = parts[2];
        if (clockTime.length() == 8 && clockTime[2] == ':' && clockTime[5] == ':') {
          Serial.print(F("🕒 Clock Time: "));
          Serial.println(clockTime);
        } else {
          Serial.println(F("❌ Invalid time format"));
        }
      }
    } else if (component == "sound") {
      if (parts[1] == "ON") {
        normal_mode();
        Serial.println(F("🔊 Sound ON"));
      } else if (parts[1] == "OFF") {
        sound_system_off();
        Serial.println(F("🔇 Sound OFF"));
      } else if (parts[1] == "BOOST") {
        soundBoost = true;
        party_mode();
        Serial.println(F("🚀 Sound Boost"));
      }
    } else if (component == "box") {
      if (parts[1] == "OPEN") {
        open_box();
        boxOpen = true;
      } else if (parts[1] == "CLOSE") {
        close_box();
        boxOpen = false;
      }
      Serial.print(F("📦 Box: "));
      Serial.println(parts[1]);
    }else if (component == "readingl"){
      if (parts[1] == "ON"){
        Serial.println(F("Executing readinglight ON"));
        readingLight(true);
        readingLightOn = true;
      } else if (parts[1] == "off"){
        Serial.println(F("Executing readinglight OFF"));
        readingLight(false);
        readingLightOn = false;
      }
      Serial.print(F("🔦 Reading Light: ")); Serial.println(parts[1]);
    }else if (component == "backl"){
      if (parts[1] == "ON"){
        Serial.println(F("Executing backlight ON"));
        backLight(true);
        backLightOn = true;
      } else if (parts[1] == "off"){
        Serial.println(F("Executing backlight OFF"));
        backLight(false);
        backLightOn = false;
      }
      Serial.print(F("🔦 Back Light: ")); Serial.println(parts[1]);
    }else {
      Serial.print(F("❌ Unknown component: "));
      Serial.println(component);
    }
  } else {
    Serial.println(F("❌ Incomplete structured command"));
  }
}



// ... (بقیه توابع equalizer, modes, box, wake word, parseRGBCommand, hexToCRGB از متن شما حفظ شده - فرض کنید کامل است)
/*void handleSerialCommand(String command) {
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
}*/

// ------------------- Equalizer Functions -------------------
void runEqualizer1() {
  // Simple wave pattern: LEDs light up sequentially based on sound level
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.1 * raw) + (0.9 * smoothedLevel);  // Smoother transition
  int litLeds = map(smoothedLevel, 0, 4095, 0, NUM_LEDS);
  litLeds = constrain(litLeds, 0, NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < litLeds) {
      uint8_t hue = colorIndex + i * 10;  // Wider color spread
      leds[i] = ColorFromPalette(currentPalette, hue, 255, currentBlending);
    } else {
      leds[i] = CRGB::Black;
    }
  }
  colorIndex += 2;  // Faster color cycling
  FastLED.show();
}

void runEqualizer2() {
  // Pulse effect: All LEDs pulse with same color based on sound level
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel);  // Very smooth
  uint8_t brightness = map(smoothedLevel, 0, 4095, 0, 255);
  brightness = constrain(brightness, 0, 255);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
  }
  colorIndex++;  // Slow color cycling
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
  smoothedLevel = (0.1 * raw) + (0.9 * smoothedLevel);  // Smoother transition
  int litLeds = map(smoothedLevel, 0, 4095, 0, NUM_LEDS);
  litLeds = constrain(litLeds, 0, NUM_LEDS);

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i < litLeds) {
      uint8_t hue = colorIndex + i * 10;  // Wider color spread
      leds[i] = ColorFromPalette(currentPalette, hue, 255, currentBlending);
    } else {
      leds[i] = CRGB::Black;
    }
  }
  colorIndex += 2;  // Faster color cycling
  FastLED.show();
}

void runBOXEqualizer2() {
  // Pulse effect: All LEDs pulse with same color based on sound level
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel);  // Very smooth
  uint8_t brightness = map(smoothedLevel, 0, 4095, 0, 255);
  brightness = constrain(brightness, 0, 255);

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
  }
  colorIndex++;  // Slow color cycling
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
  digitalWrite(MUTE, LOW);
  digitalWrite(PARTY, LOW);
  Serial.println(F("🔇 Sound System OFF (Mute + PartyOff)"));
}

void normal_mode() {
  digitalWrite(MUTE, HIGH);
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


// ------------------- Reading Light -------------------
void readingLight(bool state) {
  GPIO(READINGLIGHT, state);  // Index 1 corresponds to READINGLIGHT (pin 17) in GPIOPins
  if (state) {
    Serial.println(F("🔦 Reading Light ON"));
  } else {
    Serial.println(F("🔦 Reading Light OFF"));
  }
}

void backLight(bool state) {
  GPIO(BACKLIGHT, state);  // Index 1 corresponds to READINGLIGHT (pin 17) in GPIOPins
  if (state) {
    Serial.println(F("🔦 Reading Light ON"));
  } else {
    Serial.println(F("🔦 Reading Light OFF"));
  }
}