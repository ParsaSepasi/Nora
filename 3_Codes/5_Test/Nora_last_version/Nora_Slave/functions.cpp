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
    //Serial.println(F("Invalid command format"));
    return;
  }

  String component = toLowerCaseString(parts[0]);
  String action = toLowerCaseString(parts[1]);
  String parameter = (partCount >= 3) ? toLowerCaseString(parts[2]) : "";

  // Handle magicbl commands
  if (component == "magicbl") {
    //Serial.println(F("Processing magicbl in Slave..."));
    ledComponent = "magicbl";

    if (action == "mode") {
      if (parameter == "off") {
        ledMode = "off";
        RainbowActive = false;
        StaticActive = false;
        //Serial.println(F("magicbl Off"));
        FastLED.clear();
        FastLED.show();
      } else if (parameter == "rainbow") {
        ledMode = "rainbow";
        RainbowActive = true;
        StaticActive = false;
        //Serial.println(F("magicbl Rainbow On"));
      } else if (parameter == "static") {
        if (partCount >= 4) {
          ledColor = parts[3];
          ledMode = "static";
          RainbowActive = false;
          StaticActive = true;
          //Serial.print(F("magicbl Static On - Color: "));
          //Serial.println(ledColor);
        } else {
          //Serial.println(F("Static requires color!"));
        }
      } else {
        //Serial.println(F("Unknown magicbl mode"));
      }
    } else if (action == "brightness") {
      if (partCount >= 3) {
        int level = parts[2].toInt();
        if (level >= 0 && level <= 2) {
          brightnessLevel = level;
          //Serial.print(F("magicbl Brightness set to: "));
          //Serial.println(brightnessLevel);
        } else {
          //Serial.println(F("Invalid brightness level (0-2)"));
        }
      }
    } else {
      //Serial.println(F("Unknown magicbl action"));
    }
  } 
  // Handle clock commands
  else if (component == "clock") {
    if (action == "set") {
      if (partCount >= 3) {
        clockTime = parts[2]; // xx:xx:xx
        //Serial.print(F("Clock set to: "));
        //Serial.println(clockTime);
      } else {
        Serial.println(F("Clock set requires time!"));
      }
    } else {
      //Serial.println(F("Unknown clock action"));
    }
  } else {
    //Serial.println(F("Unknown component"));
  }
}

// ------------------- Equalizer Functions -------------------
void runRainbow() {
  //Serial.println(F("Running Rainbow Mode - Starting"));
  static uint8_t startIndex = 0;
  startIndex = startIndex + 1;
  for (int i = 0; i < NUM_BOX_LEDS; i++) {
    box_leds[i] = ColorFromPalette(currentPalette, startIndex + i * 255 / NUM_BOX_LEDS, 255, currentBlending);
  }
  FastLED.show();
  //Serial.println(F("Rainbow Updated"));
  delay(20);
}

void runStatic() {
  //Serial.println(F("Running Static"));
  CRGB color = hexToCRGB(ledColor);
  fill_solid(box_leds, NUM_BOX_LEDS, color);
  FastLED.show();
}