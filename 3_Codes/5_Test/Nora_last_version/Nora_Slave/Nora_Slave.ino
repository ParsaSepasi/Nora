#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include <esp_task_wdt.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

// تابع ارسال داده به MAX7219
void shift(byte send_to_address, byte send_this_data) {
  digitalWrite(MAX7219_Chip_Select, LOW);
  shiftOut(MAX7219_Data_IN, MAX7219_Clock, MSBFIRST, send_to_address);
  shiftOut(MAX7219_Data_IN, MAX7219_Clock, MSBFIRST, send_this_data);
  digitalWrite(MAX7219_Chip_Select, HIGH);
}

// متغیرهای زمان
unsigned long lastUpdate = 0;
int hours = 12, minutes = 34, seconds = 0;

// Callback for received data
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (len > 100) {
    Serial.println("Error: Received data too long");
    return;
  }
  String command = String((char *)data, len);
  command.trim();
  Serial.print("Received command: ");
  Serial.println(command);

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
        Serial.println("Error: CRC Mismatch");
        fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
        FastLED.show();
        delay(500);
        FastLED.clear();
        FastLED.show();
        return;
      }
      Serial.println("CRC OK");
      command = payload;
    } else {
      Serial.println("Error: Invalid CRC format");
      fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
      FastLED.show();
      delay(500);
      FastLED.clear();
      FastLED.show();
      return;
    }
  } else {
    Serial.println("Error: No CRC provided");
    fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
    FastLED.show();
    delay(500);
    FastLED.clear();
    FastLED.show();
    return;
  }

  // پردازش دستورات clock و magicbl
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

  Serial.print("Parsed parts: ");
  for (int i = 0; i < partCount; i++) {
    Serial.print(parts[i]);
    Serial.print(" ");
  }
  Serial.println();

  if (partCount < 2) {
    Serial.println("Error: Invalid command format");
    return;
  }

  String component = toLowerCaseString(parts[0]);
  String action = toLowerCaseString(parts[1]);

  if (component == "clock" && action == "time" && partCount >= 3) {
    String timeStr = parts[2]; // فرمت: HH:MM:SS
    if (timeStr.length() == 8 && timeStr[2] == ':' && timeStr[5] == ':') {
      hours = timeStr.substring(0, 2).toInt();
      minutes = timeStr.substring(3, 5).toInt();
      seconds = timeStr.substring(6, 8).toInt();
      if (hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60 && seconds >= 0 && seconds < 60) {
        Serial.print("Clock set to: ");
        Serial.println(timeStr);
        updateDisplay();
        // دیباگ با LED: آبی شدن LEDها هنگام دریافت دستور clock
        fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Blue);
        FastLED.show();
        delay(200);
        FastLED.clear();
        FastLED.show();
      } else {
        Serial.println("Error: Invalid time format");
      }
    } else {
      Serial.println("Error: Invalid time string format");
    }
  } else if (component == "magicbl") {
    handleSerialCommand(command);
  } else {
    Serial.println("Error: Unknown component or action");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Nora Slave Starting...");

  pinMode(MAX7219_Data_IN, OUTPUT);
  pinMode(MAX7219_Chip_Select, OUTPUT);
  pinMode(MAX7219_Clock, OUTPUT);
  digitalWrite(MAX7219_Clock, HIGH);
  delay(200);

  // تنظیمات اولیه MAX7219 (برای 4 رقم)
  shift(0x0f, 0x00); // خاموش کردن حالت تست
  shift(0x0c, 0x01); // حالت نرمال
  shift(0x0b, 0x03); // اسکن فقط 4 رقم (0-3)
  shift(0x0a, 0x08); // شدت نور متوسط
  shift(0x09, 0x0f); // فعال کردن decode mode برای 4 رقم

  // نمایش زمان اولیه
  updateDisplay();

  WiFi.mode(WIFI_STA);
  delay(500);

  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, BOX_PIN, COLOR_ORDER>(box_leds, NUM_BOX_LEDS);
  FastLED.setBrightness(customBrightness);
  FastLED.clear(true);
  FastLED.show();

  // Initialize states
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  RainbowActive = false;
  StaticActive = false;
  ledMode = "off";
  ledComponent = "magicbl";

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error: ESP-NOW initialization failed");
    fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
    FastLED.show();
    delay(1000);
    FastLED.clear();
    FastLED.show();
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void updateDisplay() {
  // نمایش ساعت و دقیقه: HH:MM
  // فرمت: رقم 4 (دهگان ساعت)، رقم 3 (یکان ساعت)، رقم 2 (یکان دقیقه با نقطه)، رقم 1 (دهگان دقیقه)
  shift(4, hours % 10);
  shift(3, hours / 10);
  shift(2, (minutes % 10) | 0x80); // افزودن نقطه اعشار
  shift(1, minutes / 10);

  // به‌روزرسانی clockTime برای سازگاری
  char timeStr[9];
  sprintf(timeStr, "%02d:%02d:%02d", hours, minutes, seconds);
  clockTime = String(timeStr);

  // دیباگ با LED: سبز شدن LEDها هنگام به‌روزرسانی نمایشگر
  fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Green);
  FastLED.show();
  delay(100);
  FastLED.clear();
  FastLED.show();

  Serial.print("Display updated: ");
  Serial.println(clockTime);
}

void loop() {
  // به‌روزرسانی ساعت هر ثانیه
  if (millis() - lastUpdate >= 1000) {
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0;
        }
      }
      updateDisplay();
    }
    lastUpdate = millis();
  }

  // Execute LED modes for magicbl
  if (ledMode != "off" && ledComponent == "magicbl") {
    if (ledMode == "rainbow" && RainbowActive) {
      runRainbow();
    } else if (ledMode == "static" && StaticActive) {
      runStatic();
    }
    FastLED.setBrightness(brightnessLevel * 85 + 50);
    FastLED.show();
  }

  delay(10);
}