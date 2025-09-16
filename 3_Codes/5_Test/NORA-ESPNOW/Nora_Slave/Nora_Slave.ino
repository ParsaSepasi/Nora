// Slave.ino - Slave ESP32 for Nora Project with ESP-NOW
// Receives and processes clock and magicB commands from Master
// Fixed for ESP32 Arduino Core 3.3.0 compatibility

// #include <esp_now.h>
// #include <WiFi.h>
// #include <FastLED.h>
// #include "pins.h"
// #include "state.h"
// #include "functions.h"

// // Callback for received data (fixed for 3.3.0)
// void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
//   String command = "";
//   for (int i = 0; i < len; i++) {
//     command += (char)data[i];
//   }
//   command.trim();
// #if DEBUG_SERIAL
//   Serial.print("Received: ");
//   Serial.println(command);
// #endif

//   // Validate CRC
//   int crcPos = command.lastIndexOf('_');
//   if (crcPos > 0 && (command.length() - crcPos - 1) == 2) {
//     String crcStr = command.substring(crcPos + 1);
//     bool isValidHex = true;
//     for (int i = 0; i < crcStr.length(); i++) {
//       char c = crcStr[i];
//       if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
//         isValidHex = false;
//         break;
//       }
//     }
//     if (isValidHex) {
//       String payload = command.substring(0, crcPos);
//       uint8_t expectedCrc = strtol(crcStr.c_str(), NULL, 16);
//       uint8_t calcCrc = crc8((uint8_t *)payload.c_str(), payload.length());
//       if (calcCrc != expectedCrc) {
//         Serial.println("❌ CRC Mismatch!");
//         return;
//       }
//       Serial.println("✅ CRC OK");
//       command = payload;
//     } else {
//       Serial.println("❌ Invalid CRC format");
//       return;
//     }
//   } else {
//     Serial.println("❌ No CRC provided");
//     return;
//   }

//   // Process only clock or magicBl commands
//   String component = toLowerCaseString(command.substring(0, command.indexOf('_')));
//   if (component == "clock" || component == "magicbl") {
//     handleSerialCommand(command);
//   } else {
//     Serial.println("Ignoring non-clock/magicBl command");
//   }
// }

// void setup() {
//   Serial.begin(115200);
//   Serial.println("Nora Slave Starting...");
//   Serial.println("Slave MAC: " + WiFi.macAddress()); // Print MAC for Master

//   // Initialize GPIO pins
//   for (int i = 0; i < NUM_PINS; i++) {
//     pinMode(GPIOPins[i], OUTPUT);
//     digitalWrite(GPIOPins[i], LOW);
//   }

//   // Initialize FastLED for magicB
//   FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
//   FastLED.setBrightness(customBrightness);
//   FastLED.clear(true);

//   // Initialize states
//   normal_mode();
//   currentPalette = RainbowColors_p;
//   currentBlending = LINEARBLEND;
//   pinMode(READINGLIGHT, OUTPUT);
//   digitalWrite(READINGLIGHT, LOW);
//   RainbowActive = false;
//   EqualizeActive = false;
//   StaticActive = false;
//   boxRainbowActive = false;
//   boxEqualizeActive = false;
//   boxStaticActive = false;
//   ledMode = "off";
//   ledComponent = "magicbl"; // Default for LED control

//   // Initialize ESP-NOW
//   WiFi.mode(WIFI_STA);
//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP-NOW Init Fail");
//     return;
//   }
//   esp_now_register_recv_cb(OnDataRecv);
//   Serial.println("Slave Ready - Waiting for commands");
// }

// void loop() {
//   // Execute LED modes for magicB
//   if (ledMode != "off" && ledComponent == "magicbl") {
//     if (ledMode == "rainbow" && RainbowActive) {
//       runRainbow();
//     } else if (ledMode == "equalize" && EqualizeActive) {
//       runEqualize();
//     } else if (ledMode == "static" && StaticActive) {
//       runStatic();
//     }
//     FastLED.setBrightness(brightnessLevel * 85 + 50);
//     FastLED.show();
//   } else {
//     FastLED.clear();
//     FastLED.show();
//   }

//   // Relay Timeout
//   if (relayActive && millis() - relayOnTime >= 8000) {
//     digitalWrite(OPEN_BOX, LOW);
//     digitalWrite(CLOSE_BOX, LOW);
//     relayActive = false;
//     Serial.println("Relay Off");
//   }

//   delay(10);
// }

// Slave.ino - Slave ESP32 for Nora Project with ESP-NOW
// Receives and processes clock and magicB commands from Master
// Displays clockTime on MAX7219 Dot Matrix
// Fixed for ESP32 Arduino Core 3.3.0 compatibility

#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

// MAX7219 Configuration
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW // Change to your module type (e.g., PAROLA_HW, GENERIC_HW)
#define MAX_DEVICES 4                     // Number of 8x8 modules (e.g., 4 for 32x8)
#define CLK_PIN 27                        // SPI Clock pin
#define DATA_PIN 26                       // SPI Data (MOSI) pin
#define CS_PIN 25                          // Chip Select pin
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// ESP-NOW Callback for receiving commands
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  String command = "";
  for (int i = 0; i < len; i++) {
    command += (char)data[i];
  }
  command.trim();
#if DEBUG_SERIAL
  Serial.print("Received: ");
  Serial.println(command);
#endif

  // Validate CRC
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
        Serial.println("❌ CRC Mismatch!");
        return;
      }
      Serial.println("✅ CRC OK");
      command = payload;
    } else {
      Serial.println("❌ Invalid CRC format");
      return;
    }
  } else {
    Serial.println("❌ No CRC provided");
    return;
  }

  // Process clock or magicB commands
  String component = toLowerCaseString(command.substring(0, command.indexOf('_')));
  if (component == "clock" || component == "magicb") {
    handleSerialCommand(command);
    // Update display if clock command
    if (component == "clock") {
      P.displayText(clockTime.c_str(), PA_CENTER, 100, 0, PA_PRINT, PA_NO_EFFECT);
      P.displayAnimate(); // Update display
#if DEBUG_SERIAL
      Serial.print("Clock updated on MAX7219: ");
      Serial.println(clockTime);
#endif
    }
  } else {
    Serial.println("Ignoring non-clock/magicB command");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Nora Slave Starting...");
  Serial.println("Slave MAC: " + WiFi.macAddress()); // Print MAC for Master

  // Initialize GPIO pins
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(GPIOPins[i], OUTPUT);
    digitalWrite(GPIOPins[i], LOW);
  }

  // Initialize FastLED for magicB
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(customBrightness);
  FastLED.clear(true);

  // Initialize MAX7219
  P.begin();
  P.setIntensity(8); // Brightness (0-15)
  P.displayClear();
  P.displayText("Nora Slave", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_NO_EFFECT);
  P.displayAnimate(); // Show initial message

  // Initialize states
  normal_mode();
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  pinMode(READINGLIGHT, OUTPUT);
  digitalWrite(READINGLIGHT, LOW);
  RainbowActive = false;
  EqualizeActive = false;
  StaticActive = false;
  boxRainbowActive = false;
  boxEqualizeActive = false;
  boxStaticActive = false;
  ledMode = "off";
  ledComponent = "magicb"; // Default for LED control

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Init Fail");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Slave Ready - Waiting for commands");
}

void loop() {
  // Update MAX7219 display
  if (P.displayAnimate()) {
    P.displayReset(); // Restart animation if needed
  }

  // Execute LED modes for magicB
  if (ledMode != "off" && ledComponent == "magicb") {
    if (ledMode == "rainbow" && RainbowActive) {
      runRainbow();
    } else if (ledMode == "equalize" && EqualizeActive) {
      runEqualize();
    } else if (ledMode == "static" && StaticActive) {
      runStatic();
    }
    FastLED.setBrightness(brightnessLevel * 85 + 50);
    FastLED.show();
  } else {
    FastLED.clear();
    FastLED.show();
  }

  // Relay Timeout
  if (relayActive && millis() - relayOnTime >= 8000) {
    digitalWrite(OPEN_BOX, LOW);
    digitalWrite(CLOSE_BOX, LOW);
    relayActive = false;
    Serial.println("Relay Off");
  }

  delay(10);
}