void setup() {
  Serial.begin(9600);
  delay(1000); // صبر برای آماده شدن سریال
  Serial.println("ESP32-C3 Serial Test");
  Serial.println("This is a test message");
}

void loop() {
  static int counter = 0;
  Serial.print("Counter: ");
  Serial.println(counter++);
  delay(1000);
}