#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include <esp_task_wdt.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

// تعریف پین‌های MAX7219 (از pins.h)
#define MAX7219_DATA_PIN 0  // پین DIN
#define CLK_PIN          1  // پین CLK
#define CS_PIN           3  // پین CS

// تابع ارسال داده به MAX7219
void shift(byte send_to_address, byte send_this_data) {
  digitalWrite(CS_PIN, LOW);
  shiftOut(MAX7219_DATA_PIN, CLK_PIN, MSBFIRST, send_to_address);
  shiftOut(MAX7219_DATA_PIN, CLK_PIN, MSBFIRST, send_this_data);
  digitalWrite(CS_PIN, HIGH);
}

// تابع به‌روزرسانی نمایشگر MAX7219 برای نمایش HH:MM
void updateDisplay() {
  // استخراج ساعت و دقیقه از clockTime (فرمت: xx:xx:xx)
  int hours = clockTime.substring(0, 2).toInt();
  int minutes = clockTime.substring(3, 5).toInt();

  // نمایش ساعت و دقیقه: HH:MM
  // فرمت: رقم 4 (دهگان ساعت)، رقم 3 (یکان ساعت)، رقم 2 (یکان دقیقه با نقطه)، رقم 1 (دهگان دقیقه)
  shift(4, hours / 10); // دهگان ساعت
  shift(3, hours % 10); // یکان ساعت
  shift(2, (minutes % 10) | 0x80); // یکان دقیقه با نقطه اعشار
  shift(1, minutes / 10); // دهگان دقیقه

  // دیباگ با LED: سبز شدن LEDها هنگام به‌روزرسانی نمایشگر
  fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Green);
  FastLED.show();
  delay(100);
  FastLED.clear();
  FastLED.show();
}

// Callback for received data
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (len > 100) {
    return;
  }
  String command = String((char*)data, len);
  command.trim();

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
        // دیباگ با LED: قرمز شدن LEDها برای CRC نامعتبر
        fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
        FastLED.show();
        delay(500);
        FastLED.clear();
        FastLED.show();
        return;
      }
      command = payload;
    } else {
      // دیباگ با LED: قرمز شدن LEDها برای فرمت CRC نامعتبر
      fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
      FastLED.show();
      delay(500);
      FastLED.clear();
      FastLED.show();
      return;
    }
  } else {
    // دیباگ با LED: قرمز شدن LEDها برای نبود CRC
    fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
    FastLED.show();
    delay(500);
    FastLED.clear();
    FastLED.show();
    return;
  }

  // Process clock or magicbl commands
  String component = command.substring(0, command.indexOf('_'));
  component = toLowerCaseString(component);
  if (component == "clock" || component == "magicbl") {
    handleSerialCommand(command);
    if (component == "clock") {
      updateDisplay(); // به‌روزرسانی نمایشگر MAX7219
      // دیباگ با LED: آبی شدن LEDها هنگام دریافت دستور clock
      fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Blue);
      FastLED.show();
      delay(200);
      FastLED.clear();
      FastLED.show();
    }
  }
}

void setup() {
  // تنظیم پین‌های MAX7219
  pinMode(MAX7219_DATA_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  digitalWrite(CLK_PIN, HIGH);
  delay(200);

  // تنظیمات اولیه MAX7219 برای 4 رقم
  shift(0x0f, 0x00); // خاموش کردن حالت تست
  shift(0x0c, 0x01); // حالت نرمال
  shift(0x0b, 0x03); // اسکن فقط 4 رقم (0-3)
  shift(0x0a, 0x08); // شدت نور متوسط
  shift(0x09, 0x0f); // فعال کردن decode mode برای 4 رقم

  // نمایش زمان اولیه
  clockTime = "12:34:00"; // زمان اولیه برای تست
  updateDisplay();

  // غیرفعال کردن Watchdog Timer
  // esp_task_wdt_config_t wdt_config = {
  //     .timeout_ms = 30000, // 30 ثانیه
  //     .idle_core_mask = 0,
  //     .trigger_panic = false
  // };
  // esp_task_wdt_init(&wdt_config);

  // Print MAC Address
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
    // دیباگ با LED: قرمز شدن LEDها برای خطای ESP-NOW
    fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
    FastLED.show();
    delay(1000);
    FastLED.clear();
    FastLED.show();
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
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

  yield();
  delay(10);
}