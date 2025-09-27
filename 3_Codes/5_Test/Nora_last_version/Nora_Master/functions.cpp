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
      result[i] = result[i] + 32;
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
  return CRGB::Red;
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

  // Check CRC if present
  int crcPos = command.lastIndexOf('_');
  if (crcPos > 0 && (command.length() - crcPos - 1) == 2) {
    String crcStr = command.substring(crcPos + 1);
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
        Serial.println(F("CRC Mismatch!"));
        return;
      }
      Serial.println(F("CRC OK"));
      command = payload;
    }
  }

  // Split by '_' into parts
  String parts[10];
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
  if (lastIndex < command.length()) {
    parts[partCount] = command.substring(lastIndex);
    parts[partCount].trim();
    partCount++;
  }

#if DEBUG_SERIAL
  Serial.print(F("Parsed parts: "));
  for (int i = 0; i < partCount; i++) {
    Serial.print(parts[i]);
    Serial.print(F(" "));
  }
  Serial.println();
#endif

  if (partCount < 2) {
    Serial.println(F("Invalid command format"));
    return;
  }

  String component = toLowerCaseString(parts[0]);
  String action = toLowerCaseString(parts[1]);
  String parameter = (partCount >= 3) ? toLowerCaseString(parts[2]) : "";

  // Handle magicl commands (for GPIO 21 - leds)
  if (component == "magicl") {
    Serial.println(F("Processing magicl locally in Master..."));
    ledComponent = "magicl";

    if (action == "mode") {
      if (parameter == "off") {
        ledMode = "off";
        RainbowActive = false;
        EqualizeActive = false;
        StaticActive = false;
        Serial.println(F("magicl Off in Master (GPIO 21)"));
        fill_solid(leds, NUM_LEDS, CRGB::Black);  // فقط leds خاموش میشه
      } else if (parameter == "rainbow") {
        ledMode = "rainbow";
        RainbowActive = true;
        EqualizeActive = false;
        StaticActive = false;
        Serial.println(F("magicl Rainbow On in Master (GPIO 21)"));
      } else if (parameter == "equalize") {
        ledMode = "equalize";
        RainbowActive = false;
        EqualizeActive = true;
        StaticActive = false;
        Serial.println(F("magicl Equalize On in Master (GPIO 21)"));
      } else if (parameter == "static") {
        if (partCount >= 4) {
          ledColor = parts[3];
          ledMode = "static";
          RainbowActive = false;
          EqualizeActive = false;
          StaticActive = true;
          Serial.print(F("magicl Static On in Master (GPIO 21) - Color: "));
          Serial.println(ledColor);
        } else {
          Serial.println(F("Static requires color!"));
        }
      } else {
        Serial.println(F("Unknown magicl mode"));
      }
    } else if (action == "brightness") {
      if (partCount >= 3) {
        int level = parts[2].toInt();
        if (level >= 0 && level <= 2) {
          brightnessLevel = level;
          Serial.print(F("magicl Brightness set to: "));
          Serial.println(brightnessLevel);
        } else {
          Serial.println(F("Invalid brightness level (0-2)"));
        }
      }
    } else {
      Serial.println(F("Unknown magicl action"));
    }
  }
  // Handle magicbl commands (for GPIO 22 - box_leds)
  else if (component == "magicbl") {
    Serial.println(F("Processing magicbl locally in Master..."));
    ledComponent = "magicbl";

    if (action == "mode") {
      if (parameter == "off") {
        boxRainbowActive = false;
        boxEqualizeActive = false;
        boxStaticActive = false;
        Serial.println(F("magicbl Off in Master (GPIO 22)"));
        fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Black);  // فقط box_leds خاموش میشه
      } else if (parameter == "rainbow") {
        boxRainbowActive = true;
        boxEqualizeActive = false;
        boxStaticActive = false;
        Serial.println(F("magicbl Rainbow On in Master (GPIO 22)"));
      } else if (parameter == "equalize") {
        boxRainbowActive = false;
        boxEqualizeActive = true;
        boxStaticActive = false;
        Serial.println(F("magicbl Equalize On in Master (GPIO 22)"));
      } else if (parameter == "static") {
        if (partCount >= 4) {
          ledColor = parts[3];
          boxRainbowActive = false;
          boxEqualizeActive = false;
          boxStaticActive = true;
          Serial.print(F("magicbl Static On in Master (GPIO 22) - Color: "));
          Serial.println(ledColor);
        } else {
          Serial.println(F("Static requires color!"));
        }
      } else {
        Serial.println(F("Unknown magicbl mode"));
      }
    } else if (action == "brightness") {
      if (partCount >= 3) {
        int level = parts[2].toInt();
        if (level >= 0 && level <= 2) {
          brightnessLevel = level;
          Serial.print(F("magicbl Brightness set to: "));
          Serial.println(brightnessLevel);
        } else {
          Serial.println(F("Invalid brightness level (0-2)"));
        }
      }
    } else {
      Serial.println(F("Unknown magicbl action"));
    }
  } else {
    Serial.println(F("Unknown component"));
  }
  if (component == "clock") {
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
  } else if (component == "readingl") {
    if (parts[1] == "ON") {
      Serial.println(F("Executing readinglight ON"));
      readingLight(true);
      readingLightOn = true;
    } else if (parts[1] == "OFF") {
      Serial.println(F("Executing readinglight OFF"));
      readingLight(false);
      readingLightOn = false;
    }
    Serial.print(F("🔦 Reading Light: "));
    Serial.println(parts[1]);
  } else if (component == "backl") {
    if (parts[1] == "ON") {
      Serial.println(F("Executing backlight ON"));
      backLight(true);
      backLightOn = true;
    } else if (parts[1] == "OFF") {
      Serial.println(F("Executing backlight OFF"));
      backLight(false);
      backLightOn = false;
    }
    Serial.print(F("🔦 Back Light: "));
    Serial.println(parts[1]);
  } else {
    Serial.print(F("❌ Unknown component: "));
    Serial.println(component);
  }
}


// ------------------- Equalizer Functions -------------------
void runRainbow() {
  Serial.println(F("Running Rainbow (GPIO 21)"));
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, startIndex + i * 255 / NUM_LEDS, 255, currentBlending);
  }
}

void runEqualize() {
  Serial.println(F("Running Equalize (GPIO 21)"));
  int micValue = analogRead(MIC_PIN);
  uint8_t brightness = map(micValue, 0, 4095, 0, 255);
  fill_solid(leds, NUM_LEDS, CRGB(brightness, 0, 0));
}

void runStatic() {
  Serial.println(F("Running Static (GPIO 21)"));
  CRGB color = hexToCRGB(ledColor);
  fill_solid(leds, NUM_LEDS, color);
}

void runBOXRainbow() {
  Serial.println(F("Running BOX Rainbow (GPIO 22)"));
  static uint8_t hue = 0;
  fill_rainbow(box_leds, NUM_BOX_LEDS, hue, 7);
  hue += 2;
}

void runBOXEqualize() {
  Serial.println(F("Running BOX Equalize (GPIO 22)"));
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel);
  uint8_t brightness = map(smoothedLevel, 0, 4095, 0, 255);
  brightness = constrain(brightness, 0, 255);
  for (int i = 0; i < NUM_BOX_LEDS; i++) {
    box_leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
  }
  colorIndex++;
}

void runBOXStatic() {
  Serial.println(F("Running BOX Static (GPIO 22)"));
  CRGB color = hexToCRGB(ledColor);
  fill_solid(box_leds, NUM_BOX_LEDS, color);
}

// ------------------- Modes -------------------
void sound_system_off() {
  digitalWrite(MUTE, LOW);
  digitalWrite(PARTY, LOW);
  Serial.println(F("Sound System OFF (Mute + PartyOff)"));
}

void normal_mode() {
  digitalWrite(MUTE, HIGH);
  digitalWrite(PARTY, LOW);
  Serial.println(F("Normal Mode (Unmute + PartyOff)"));
}

void party_mode() {
  digitalWrite(MUTE, HIGH);
  digitalWrite(PARTY, HIGH);
  Serial.println(F("Party Mode (Unmute + PartyOn)"));
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
  GPIO(1, state);
  if (state) {
    Serial.println(F("Reading Light ON"));
  } else {
    Serial.println(F("Reading Light OFF"));
  }
}

void backLight(bool state) {
  GPIO(0, state);
  if (state) {
    Serial.println(F("Back Light ON"));
  } else {
    Serial.println(F("Back Light OFF"));
  }
}