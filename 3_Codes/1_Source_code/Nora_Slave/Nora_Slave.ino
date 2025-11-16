#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include <esp_task_wdt.h>
#include <Wire.h>
#include "RTClib.h"
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

// متغیرهای زمان (برای نمایشگر و state)
unsigned long lastUpdate = 0;
int hours = 12, minutes = 34, seconds = 0;

// Callback for received data
// void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
//   if (len > 100) {
//     Serial.println("Error: Received data too long");
//     return;
//   }
//   String command = String((char *)data, len);
//   command.trim();
//   Serial.print("Received command: ");
//   Serial.println(command);

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
//         Serial.println("Error: CRC Mismatch");
//         fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
//         FastLED.show();
//         delay(500);
//         FastLED.clear();
//         FastLED.show();
//         return;
//       } else {
//         Serial.println("CRC OK");
//         command = payload;
//       }
//     } else {
//       Serial.println("Error: Invalid CRC format");
//       fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
//       FastLED.show();
//       delay(500);
//       FastLED.clear();
//       FastLED.show();
//       return;
//     }
//   } else {
//     Serial.println("Error: No CRC provided");
//     fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
//     FastLED.show();
//     delay(500);
//     FastLED.clear();
//     FastLED.show();
//     return;
//   }

//   // پردازش دستورات clock و magicbl
//   String parts[10];
//   int partCount = 0;
//   int lastIndex = 0;
//   for (int i = 0; i < command.length() && partCount < 10; i++) {
//     if (command[i] == '_') {
//       parts[partCount] = command.substring(lastIndex, i);
//       parts[partCount].trim();
//       lastIndex = i + 1;
//       partCount++;
//     }
//   }
//   if (lastIndex < command.length()) {
//     parts[partCount] = command.substring(lastIndex);
//     parts[partCount].trim();
//     partCount++;
//   }

//   Serial.print("Parsed parts: ");
//   for (int i = 0; i < partCount; i++) {
//     Serial.print(parts[i]);
//     Serial.print(" ");
//   }
//   Serial.println();

//   if (partCount < 2) {
//     Serial.println("Error: Invalid command format");
//     return;
//   }

//   String component = toLowerCaseString(parts[0]);
//   String action = toLowerCaseString(parts[1]);

//   // clock_time_HH:MM:SS  → تنظیم ساعت
//   if (component == "clock" && action == "time" && partCount >= 3) {
//     String timeStr = parts[2]; // فرمت: HH:MM:SS
//     if (timeStr.length() == 8 && timeStr[2] == ':' && timeStr[5] == ':') {
//       hours = timeStr.substring(0, 2).toInt();
//       minutes = timeStr.substring(3, 5).toInt();
//       seconds = timeStr.substring(6, 8).toInt();
//       if (hours >= 0 && hours < 24 && minutes >= 0 && minutes < 60 && seconds >= 0 && seconds < 60) {
//         Serial.print("Clock set to: ");
//         Serial.println(timeStr);

//         // ✅ آپدیت RTC با زمان جدید، تاریخ فعلی حفظ می‌شود
//         DateTime nowRtc = rtc.now();
//         rtc.adjust(DateTime(nowRtc.year(), nowRtc.month(), nowRtc.day(),
//                             hours, minutes, seconds));

//         updateDisplay();
//         // دیباگ با LED: آبی شدن LEDها هنگام دریافت دستور clock
//         fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Blue);
//         FastLED.show();
//         delay(200);
//         FastLED.clear();
//         FastLED.show();
//       } else {
//         Serial.println("Error: Invalid time format");
//       }
//     } else {
//       Serial.println("Error: Invalid time string format");
//     }

//   } else if (component == "magicbl") {
//     // بقیه‌ی فرمان‌ها برای LED ها
//     handleSerialCommand(command);
//   } else {
//     Serial.println("Error: Unknown component or action");
//   }
// }

void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (len <= 0) return;

  String command = String((char*)data, len);
  command.trim();
  Serial.print("Received command: ");
  Serial.println(command);

  // --- چک کردن CRC مثل قبل ---
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
        Serial.println("CRC Mismatch!");
        return;
      } else {
        Serial.println("CRC OK");
        command = payload;
      }
    }
  }

  // --- Split by '_' ---
  String parts[10];
  int partCount = 0;
  int lastIndex = 0;
  for (int i = 0; i < command.length() && partCount < 10; i++) {
    if (command[i] == '_') {
      parts[partCount++] = command.substring(lastIndex, i);
      lastIndex = i + 1;
    }
  }
  if (lastIndex < command.length()) {
    parts[partCount++] = command.substring(lastIndex);
  }

  for (int i = 0; i < partCount; i++) parts[i].trim();

  if (partCount < 2) return;

  String component = toLowerCaseString(parts[0]);
  String action    = toLowerCaseString(parts[1]);

  // ---- ۱) فرمان ساعت از Master: RTC را ست کن ----
  if (component == "clock" && action == "time" && partCount >= 3) {
    String timeStr = parts[2];   // "HH:MM:SS"
    if (timeStr.length() == 8 && timeStr[2] == ':' && timeStr[5] == ':') {
      int hh = timeStr.substring(0, 2).toInt();
      int mm = timeStr.substring(3, 5).toInt();
      int ss = timeStr.substring(6, 8).toInt();

      if (hh >= 0 && hh < 24 && mm >= 0 && mm < 60 && ss >= 0 && ss < 60) {
        DateTime nowDate = rtc.now();  // تاریخ فعلی RTC را نگه می‌داریم
        rtc.adjust(DateTime(nowDate.year(), nowDate.month(), nowDate.day(),
                            hh, mm, ss));

        Serial.print("RTC Updated from ESP-NOW -> ");
        Serial.println(timeStr);

        // clockTime را هم به‌روزرسانی کن (برای state)
        char buf[9];
        sprintf(buf, "%02d:%02d:%02d", hh, mm, ss);
        clockTime = String(buf);

        // نمایشگر خودش در loop از روی RTC آپدیت می‌شود
      } else {
        Serial.println("Invalid time values");
      }
    } else {
      Serial.println("Invalid time format string");
    }
  }

  // ---- ۲) بقیه فرمان‌ها (magicbl و ...) ----
  else if (component == "magicbl") {
    handleSerialCommand(command);   // همون قبلی که داشتی
  } else {
    Serial.println("Unknown component");
  }
}

void updateDisplayFromRTC() {
  // زمان فعلی از RTC
  DateTime now = rtc.now();
  int h = now.hour();
  int m = now.minute();
  int s = now.second();

  // نمایش فرمت HH:MM روی ۴ رقم
  // فرض: 
  //  digit1 -> آدرس 1  (دهگان دقیقه)
  //  digit2 -> آدرس 2  (یکان دقیقه + نقطه)
  //  digit3 -> آدرس 3  (یکان ساعت)
  //  digit4 -> آدرس 4  (دهگان ساعت)

  shift(4, h % 10);                 // دهگان ساعت
  shift(3, h / 10);                 // یکان ساعت
  shift(2, (m % 10) | 0x80);        // یکان دقیقه + نقطه (برای جداکننده)
  shift(1, m / 10);                 // دهگان دقیقه

  // برای هم‌خوانی با state موجود
  char buf[9];
  sprintf(buf, "%02d:%02d:%02d", h, m, s);
  clockTime = String(buf);

  // فقط برای دیباگ (اگر خواستی خاموشش کن)
  Serial.print("RTC -> Display: ");
  Serial.println(clockTime);
}



void setup() {
  Serial.begin(115200);
  Serial.println("Nora Slave Starting...");

  // ✅ راه‌اندازی I2C و RTC (DS1307 روی SDA=8, SCL=9)
  Wire.begin(8, 2);
  Serial.println("I2C init on SDA=8, SCL=9");
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    fill_solid(box_leds, NUM_BOX_LEDS, CRGB::Red);
    FastLED.show();
    while (true) {
      delay(1000);
    }
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running, setting compile time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // زمان اولیه را از خود RTC بخوان و در متغیرهای نمایشگر بنشان
  DateTime nowRtc = rtc.now();
  hours   = nowRtc.hour();
  minutes = nowRtc.minute();
  seconds = nowRtc.second();

  // راه‌اندازی MAX7219
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

  // نمایش زمان اولیه (از RTC)
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

  // به‌روزرسانی clockTime برای سازگاری با state.cpp
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
static unsigned long lastRtcDisplayUpdate = 0;

  // هر یک ثانیه، نمایشگر را از روی RTC آپدیت کن
  if (millis() - lastRtcDisplayUpdate >= 1000) {
    updateDisplayFromRTC();
    lastRtcDisplayUpdate = millis();
  }

  // ... بقیه کدهای مربوط به LED و غیره
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
