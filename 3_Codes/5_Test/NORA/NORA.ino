#include <FastLED.h>

#define NUM_PINS     6
#define BACKLIGHT    16
#define READINGLIGHT 17
#define OPEN_BOX     19
#define CLOSE_BOX    21
#define MUTE         22
#define PARTY        23
#define LED_PIN      18
#define BOX_PIN      4
#define NUM_LEDS     98  
#define NUM_BOX_LEDS 15
#define LED_TYPE     WS2811
#define COLOR_ORDER  RGB

CRGB leds[NUM_LEDS];

const int GPIOPins[NUM_PINS] = {BACKLIGHT, READINGLIGHT, PARTY, MUTE,OPEN_BOX, CLOSE_BOX};
const String GPIONames[NUM_PINS] = {"BACKLIGHT", "READINGLIGHT", "PARTY", "MUTE", "OPEN_BOX", "CLOSE_BOX"};
String inputString = "";
int customBrightness = 100;
bool stringComplete = false;

bool equalizer1Active = false;
bool equalizer2Active = false;
bool equalizer3Active = false;
bool boxequalizer1Active = false;
bool boxequalizer2Active = false;
bool boxequalizer3Active = false;

unsigned long relayOnTime = 0;
bool relayActive = false;

void run_led_wake_word();


// برای Equalizer 1
CRGBPalette16 currentPalette;
TBlendType currentBlending;

// برای Equalizer 2 (رنگ سفارشی)
int customR = 255, customG = 50, customB = 0;  // رنگ پیش‌فرض

// برای Equalizer 3
#define MIC_PIN 32
float smoothedLevel = 0;
int dynamicMin = 4095;
int dynamicMax = 0;
bool dynamicRangeValid = false;
unsigned long lastCalibrate = 0;
uint8_t colorIndex = 0;



void setup(){
  Serial.begin(115200);
  for (int i = 0; i < NUM_PINS; i++){
    pinMode(GPIOPins[i], OUTPUT);
    digitalWrite(GPIOPins[i], false);
  }
  inputString.reserve(100);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(customBrightness);  // اگر خواستی متغیر بسازی

  FastLED.clear(true);

  normal_mode();

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;

}

void loop(){
  if (stringComplete){
    String command = inputString;
    command.trim();
    handleSerialCommand(command);
    inputString = "";
    stringComplete = false;
  }
  else if (Serial.available()){
    char c =Serial.read();
    if(c == 'R'){
      open_box();
      close_box();
    }
  }
  else if(relayActive && millis() - relayOnTime >= 8000){
    digitalWrite(OPEN_BOX, false);
    digitalWrite(CLOSE_BOX, false);
    relayActive = false;
    Serial.println("Relay auto off after 8s");
  }
  else if (equalizer1Active) {runEqualizer1();}
  else if (equalizer2Active) {runEqualizer2();}
  else if (equalizer3Active) {runEqualizer3();}
}

void serialEvent(){
  while (Serial.available()){
    char inChar = (char)Serial.read();
    if(inChar >= 32 && inChar<=126){
      inputString += inChar;
    } else if (inChar =='\n' || inChar == '\r'){
      stringComplete = true;
    }
  }
}

void GPIO(int GPIOIndex, bool state){
  if (GPIOIndex >= 0 && GPIOIndex < NUM_PINS) {
    int pin = GPIOPins[GPIOIndex];
    digitalWrite(pin, state ? true : false);
    Serial.println(state ? "ON" + GPIONames[GPIOIndex] + "on GPIO" + String(pin) +"ON" : "OFF" +GPIONames[GPIOIndex] + "onGPIO" +String(pin)+ "OFF");
    }else{
      Serial.println("Invalid GPIO index" + String(GPIOIndex));
    }
  }


void handleSerialCommand(String command) {
  command.toUpperCase(); // تبدیل به حروف بزرگ
  String norm = command;
  norm.trim();
  norm.toUpperCase();
  String noSpace = norm;
  noSpace.replace(" ", "");
  if (command == "EQUALIZER 1") {
    equalizer1Active = true;
    equalizer2Active = false;
    equalizer3Active = false;
    Serial.println("🎛️ EQ1 Started");
  } else if (command == "EQUALIZER 2") {
    equalizer1Active = false;
    equalizer2Active = true;
    equalizer3Active = false;
    Serial.println("🎛️ EQ2 Started");
  } else if (command == "EQUALIZER 3") {
    equalizer1Active = false;
    equalizer2Active = false;
    equalizer3Active = true;
    smoothedLevel = 0;
    dynamicMin = 4095;
    dynamicMax = 0;
    dynamicRangeValid = false;
    lastCalibrate = millis();
    Serial.println("🎛️ EQ3 Started");
  } else if (command == "EQUALIZER OFF") {
    equalizer1Active = false;
    equalizer2Active = false;
    equalizer3Active = false;
    FastLED.clear(true);
    Serial.println("🛑 EQ OFF");
  }else if (command == "WAKE WORD"){
    run_led_wake_word();

    Serial.println("🎛️ wake word Started");
  }else
  for(int i = 0; i<NUM_PINS; i++){
    String onCmd = GPIONames[i] + "_ON";
    String offCmd = GPIONames[i] + "_OFF";
    if (command == onCmd){
      GPIO(i,true);
      return;
    } else if (command == offCmd){
      GPIO(i, false);
      return;
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

    brightness = constrain(brightness, 0, 100);  // محدود کردن بازه
    FastLED.setBrightness(brightness);

    customR = r;
    customG = g;
    customB = b;

    Serial.printf("🎨 RGB set: R=%d, G=%d, B=%d | Brightness=%d\n", r, g, b, brightness);
  }
}

// Equalizer 1: رنگین‌کمان
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

// Equalizer 2: رنگ ثابت سفارشی
void runEqualizer2() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(customR, customG, customB);
  }
  FastLED.show();
  delay(30);
}

// Equalizer 3: میکروفون رنگین‌کمانی
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
    leds[mid + i]     = ColorFromPalette(RainbowColors_p, hue);
  }

  colorIndex++;
  FastLED.show();
}

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

void open_box(){
  digitalWrite(OPEN_BOX, true);
  relayOnTime = millis();//time to open the Box
  relayActive = true;
//  void party_mode();
  Serial.println(F("box opened"));

}

void close_box(){
  digitalWrite(CLOSE_BOX, true);
  relayOnTime = millis();//time to open the Box
  relayActive = true;
//  void normal_mode();
  Serial.println(F("box closed"));
}

void run_led_wake_word() {
  const int sections = 5;                      // 5 section
  int ledsPerSection = NUM_LEDS / sections;
  CRGB wakeColor = CRGB(0, 255, 255);          // فیروزه‌ای

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
