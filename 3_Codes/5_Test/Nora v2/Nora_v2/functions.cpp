#include <FastLED.h>
#include "pins.h"
#include "state.h"
#include "functions.h"

// ------------------- GPIO -------------------
void GPIO(int GPIOIndex, bool state){
  if (GPIOIndex >= 0 && GPIOIndex < NUM_PINS) {
    int pin = GPIOPins[GPIOIndex];
    digitalWrite(pin, state ? true : false);
    Serial.println(state ? "ON" + GPIONames[GPIOIndex] + "on GPIO" + String(pin) +"ON" : "OFF" +GPIONames[GPIOIndex] + "onGPIO" +String(pin)+ "OFF");
  } else {
    Serial.println("Invalid GPIO index" + String(GPIOIndex));
  }
}

// ------------------- دستورات سریال -------------------
void handleSerialCommand(String command) {
  command.toUpperCase();
  String norm = command;
  norm.trim();
  norm.toUpperCase();
  String noSpace = norm;
  noSpace.replace(" ", "");

  if (command == "EQUALIZER 1") {
    equalizer1Active = true; equalizer2Active = false; equalizer3Active = false;
    Serial.println("🎛️ EQ1 Started");
  } else if (command == "EQUALIZER 2") {
    equalizer1Active = false; equalizer2Active = true; equalizer3Active = false;
    Serial.println("🎛️ EQ2 Started");
  } else if (command == "EQUALIZER 3") {
    equalizer1Active = false; equalizer2Active = false; equalizer3Active = true;
    smoothedLevel = 0; dynamicMin = 4095; dynamicMax = 0; dynamicRangeValid = false; lastCalibrate = millis();
    Serial.println("🎛️ EQ3 Started");
  } else if (command == "EQUALIZER OFF") {
    equalizer1Active = false; equalizer2Active = false; equalizer3Active = false;
    FastLED.clear(true);
    Serial.println("🛑 EQ OFF");
  } else if (command == "WAKE WORD") {
    run_led_wake_word();
    Serial.println("🎛️ wake word Started");
  } else {
    for(int i = 0; i<NUM_PINS; i++){
      String onCmd = GPIONames[i] + "_ON";
      String offCmd = GPIONames[i] + "_OFF";
      if (command == onCmd){
        GPIO(i,true); return;
      } else if (command == offCmd){
        GPIO(i, false); return;
      }
    }
  }

  if (command.startsWith("RGB:")) {
    parseRGBCommand(command.substring(4));
  }

  if (noSpace == "SOUND_OFF") { sound_system_off(); return; }
  if (noSpace == "NORMAL")    { close_box(); return; }
  if (noSpace == "PARTY")     { open_box(); return; }
  else {Serial.println("❌ Unknown command: " + command);}
}

// ------------------- RGB -------------------
void parseRGBCommand(String rgbString) {
  int r = 0, g = 0, b = 0, brightness = 100;
  int idx1 = rgbString.indexOf(',');
  int idx2 = rgbString.indexOf(',', idx1 + 1);
  int idx3 = rgbString.indexOf(',', idx2 + 1);
  if (idx1 > 0 && idx2 > idx1 && idx3 > idx2) {
    r = rgbString.substring(0, idx1).toInt();
    g = rgbString.substring(idx1 + 1, idx2).toInt();
    b = rgbString.substring(idx2 + 1, idx3).toInt();
    brightness = rgbString.substring(idx3 + 1).toInt();
    brightness = constrain(brightness, 0, 100);
    FastLED.setBrightness(brightness);
    customR = r; customG = g; customB = b;
    Serial.printf("🎨 RGB set: R=%d, G=%d, B=%d | Brightness=%d\n", r, g, b, brightness);
  }
}

// ------------------- EQ -------------------
void runEqualizer1() {
  Serial.println("runEqualizer1");
  static uint8_t startIndex = 0;
  startIndex++;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, startIndex + i, 255, currentBlending);
  }
  FastLED.show();
  delay(15);
}
void runEqualizer2() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(customR, customG, customB);
  }
  FastLED.show();
  delay(30);
}
void runEqualizer3() {
  static unsigned long lastUpdate = 0;
  const unsigned long updateInterval = 10;
  if (millis() - lastUpdate < updateInterval) return;
  lastUpdate = millis();
  int raw = analogRead(MIC_PIN);
  smoothedLevel = (0.05 * raw) + (0.95 * smoothedLevel);
  if (smoothedLevel < dynamicMin) dynamicMin = smoothedLevel;
  if (smoothedLevel > dynamicMax) dynamicMax = smoothedLevel;
  if (millis() - lastCalibrate >= 3000) {
    int range = dynamicMax - dynamicMin;
    dynamicRangeValid = (range >= 25);
    dynamicMin = 4095; dynamicMax = 0; lastCalibrate = millis();
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
    leds[mid + i]     = ColorFromPalette(RainbowColors_p, hue);
  }
  colorIndex++;
  FastLED.show();
}

// ------------------- Modes -------------------
void sound_system_off(){
  digitalWrite(MUTE, true);
  digitalWrite(PARTY, false);
  Serial.println(F("🔇 Sound System OFF (Mute + PartyOff)"));
}
void normal_mode(){
  digitalWrite(MUTE, false);
  digitalWrite(PARTY, false);
  Serial.println(F("🎵 Normal Mode (Unmute + PartyOff)"));
}
void party_mode(){
  digitalWrite(MUTE, false);
  digitalWrite(PARTY, true);
  Serial.println(F("🎉 Party Mode (Unmute + PartyOn)"));
}

// ------------------- Box -------------------
void open_box(){
  digitalWrite(OPEN_BOX, true);
  relayOnTime = millis();
  relayActive = true;
  Serial.println(F("box opened"));
}
void close_box(){
  digitalWrite(CLOSE_BOX, true);
  relayOnTime = millis();
  relayActive = true;
  Serial.println(F("box closed"));
}

// ------------------- Wake word -------------------
void run_led_wake_word() {
  const int sections = 5;
  int ledsPerSection = NUM_LEDS / sections;
  CRGB wakeColor = CRGB(0, 255, 255);
  Serial.println(F("wake word mige salam"));
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
