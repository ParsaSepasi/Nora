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
    Serial.print(F("Component: '"));
    Serial.print(component);
    Serial.println(F("'"));
    if (component == "magicl" || component == "magicbl") {
      ledComponent = component;
      if (partCount >= 3 && parts[1] == "MODE") {
        ledMode = toLowerCaseString(parts[2]);
        Serial.print(F("ledMode set to: '"));
        Serial.print(ledMode);
        Serial.println(F("'"));
        if (ledMode == "off") {
          FastLED.clear(true);
          Serial.println(F("🛑 LED Off"));
          RainbowActive = false;
          EqualizeActive = false;
          StaticActive = false;
        } else if (ledMode == "equalize") {
          if (partCount >= 4 && parts[3].toInt() > 0) {
            EqualizeActive = true;
            Serial.print(F("🎛️ Equalizer Mode Level: "));
            Serial.println(parts[3]);
          } else {
            EqualizeActive = true;
            Serial.println(F("🎛️ Equalizer Mode (Level 1)"));
          }
          RainbowActive = false;
          StaticActive = false;
        } else if (ledMode == "wakeup") {
          run_led_wake_word();
          Serial.println(F("🌅 Wakeup Mode"));
          RainbowActive = false;
          EqualizeActive = false;
          StaticActive = false;
        } else if (ledMode == "rainbow") {
          RainbowActive = true;
          currentPalette = RainbowColors_p;
          EqualizeActive = false;
          StaticActive = false;
          Serial.println(F("🌈 Rainbow Mode Activated"));
        } else if (ledMode == "static") {
          StaticActive = true;
          if (partCount >= 4 && parts[3].startsWith("#")) {
            ledColor = parts[3];
            parseRGBCommand(ledColor);
          }
          RainbowActive = false;
          EqualizeActive = false;
          Serial.println(F("🟥 Static Mode Activated"));
        } else {
          Serial.print(F("❌ Unknown ledMode: '"));
          Serial.print(ledMode);
          Serial.println(F("'"));
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
  } else {
    Serial.println(F("❌ Incomplete structured command"));
  }
}

// ------------------- Equalizer Functions -------------------
void runRainbow() {
  // Serial.println(F("running Rainbow"));
  // static uint8_t startIndex = 0;
  // startIndex++;
  // for (int i = 0; i < NUM_LEDS; i++) {
  //   leds[i] = ColorFromPalette(currentPalette, startIndex + i, 255, currentBlending);
  // }
  // FastLED.show();
  // delay(15);
  static uint8_t hue = 0;
  fill_rainbow(leds, NUM_LEDS, hue, 6);
  hue += 2;
}

// void runEqualize() {
//   // Pulse effect: All LEDs pulse with same color based on sound level
//   int raw = analogRead(MIC_PIN);
//   smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel);  // Very smooth
//   uint8_t brightness = map(smoothedLevel, 0, 4095, 0, 255);
//   brightness = constrain(brightness, 0, 255);

//   for (int i = 0; i < NUM_LEDS; i++) {
//     leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
//   }
//   colorIndex++;  // Slow color cycling
//   FastLED.show();
// }

void runEqualize() {
  Serial.println(F("Running Equalize"));
  // نمونه ساده: پر کردن LEDها با رنگ متغیر بر اساس میکروفون
  int micValue = analogRead(MIC_PIN);
  uint8_t brightness = map(micValue, 0, 4095, 0, 255);
  fill_solid(leds, NUM_LEDS, CRGB(brightness, 0, 0));  // قرمز متغیر
  FastLED.show();
  delay(10);
}

void runStatic() {
  Serial.println(F("Running Static"));
  CRGB color = hexToCRGB(ledColor);   // تبدیل رنگ هگز به CRGB
  fill_solid(leds, NUM_LEDS, color);  // پر کردن LEDها با رنگ ثابت
  FastLED.show();
}

void runBOXRainbow() {
  Serial.println("runBOXRainbow");
  static uint8_t startIndex = 0;
  startIndex++;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, startIndex + i, 255, currentBlending);
  }
  FastLED.show();
  delay(15);
}

void runBOXEqualize() {
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

void runBOXStatic() {
  Serial.println(F("Running Static"));
  CRGB color = hexToCRGB(ledColor);   // تبدیل رنگ هگز به CRGB
  fill_solid(leds, NUM_LEDS, color);  // پر کردن LEDها با رنگ ثابت
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
  digitalWrite(MUTE, HIGH);
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
  GPIO(1, state);  // Index 1 corresponds to READINGLIGHT (pin 17) in GPIOPins
  if (state) {
    Serial.println(F("🔦 Reading Light ON"));
  } else {
    Serial.println(F("🔦 Reading Light OFF"));
  }
}

void backLight(bool state) {
  GPIO(0, state);  // Index 0 corresponds to BACKLIGHT (pin 16) in GPIOPins
  if (state) {
    Serial.println(F("🔦 Reading Light ON"));
  } else {
    Serial.println(F("🔦 Reading Light OFF"));
  }
}