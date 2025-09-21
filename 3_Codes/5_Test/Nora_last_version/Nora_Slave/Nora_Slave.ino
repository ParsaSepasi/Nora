#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW  // نوع سخت‌افزار MAX7219
MD_Parola P = MD_Parola(HARDWARE_TYPE, MAX7219_DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Callback for received data
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (len > 100) {  // لیمیت برای جلوگیری از flood
    Serial.println(F("Received data too long!"));
    return;
  }
  String command = String((char*)data, len);
  command.trim();
#if DEBUG_SERIAL
  Serial.print(F("Received: "));
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
        Serial.println(F("❌ CRC Mismatch!"));
        return;
      }
      Serial.println(F("✅ CRC OK"));
      command = payload;
    } else {
      Serial.println(F("❌ Invalid CRC format"));
      return;
    }
  } else {
    Serial.println(F("❌ No CRC provided"));
    return;
  }

  // Process clock or magicbl or box commands
  String component = command.substring(0, command.indexOf('_'));
  component = toLowerCaseString(component);  // Fixed: Properly assign the lowercase string
  if (component == "clock" || component == "magicbl" || component == "box") {
    handleSerialCommand(command);
    if (component == "clock") {
      P.displayText(clockTime.c_str(), PA_CENTER, 100, 0, PA_PRINT, PA_NO_EFFECT);
      P.displayAnimate();
#if DEBUG_SERIAL
      Serial.print(F("Clock updated on MAX7219: "));
      Serial.println(clockTime);
#endif
    }
  } else {
    Serial.println(F("Ignoring non-clock/magicbl/box command"));
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Nora Slave Starting...");
  Serial.println("Slave MAC: " + WiFi.macAddress());

  // Initialize GPIO pins
  for (int i = 0; i < NUM_PINS; i++) {
    pinMode(GPIOPins[i], OUTPUT);
    digitalWrite(GPIOPins[i], LOW);
  }

  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.addLeds<LED_TYPE, BOX_PIN, COLOR_ORDER>(box_leds, NUM_BOX_LEDS);
  FastLED.setBrightness(customBrightness);
  FastLED.clear(true);
  FastLED.show();  // اضافه شده برای اطمینان از پاک کردن اولیه LEDها

  // Initialize MAX7219
  P.begin();
  P.setIntensity(8);
  P.displayClear();
  P.displayText("Nora Slave", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_NO_EFFECT);
  P.displayAnimate();

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
  ledComponent = "magicbl";

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println(F("ESP-NOW Init Fail"));
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println(F("Slave Ready - Waiting for commands"));
}

void loop() {
  // Update MAX7219 display
  if (P.displayAnimate()) {
    P.displayReset();
  }

  // Execute LED modes for magicbl
  if (ledMode != "off" && ledComponent == "magicbl") {
    Serial.println(F("Updating magicbl LEDs..."));  // دیباگ: چک کن آیا این پیام ظاهر می‌شه
    Serial.print(F("ledMode: ")); Serial.println(ledMode);  // دیباگ
    Serial.print(F("RainbowActive: ")); Serial.println(RainbowActive);  // دیباگ
    if (ledMode == "rainbow" && RainbowActive) {
      runRainbow();
    } else if (ledMode == "equalize" && EqualizeActive) {
      runEqualize();
    } else if (ledMode == "static" && StaticActive) {
      runStatic();
    }
    FastLED.setBrightness(brightnessLevel * 85 + 50);
    FastLED.show();
  } else if (ledMode != "off" && ledComponent == "box") {
    Serial.println(F("Updating box LEDs..."));  // دیباگ
    if (ledMode == "rainbow" && boxRainbowActive) {
      runBOXRainbow();
    } else if (ledMode == "equalize" && boxEqualizeActive) {
      runBOXEqualize();
    } else if (ledMode == "static" && boxStaticActive) {
      runBOXStatic();
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
    Serial.println(F("Relay Off"));
  }

  delay(10);
}