/*
  Unbent: pressure sensor test (no resistor needed)

  WIRING (male-to-female wires)
    sensor leg 1 -> j21  (A0)
    sensor leg 2 -> j11  (GND)
    Either leg can go either way.

  HOW IT WORKS
    The Nano's built-in pull-up keeps A0 HIGH. Pressing the sensor
    pulls A0 LOW, so the sketch reads "PRESSED".

  USE
    Upload, then Tools > Serial Monitor at 115200 baud.
    Press the sensor: the orange "L" LED near the USB lights up and the
    count goes up. Tools > Serial Plotter shows it as a graph (0 or 100).
*/

const int SENSOR_PIN = A0;

bool wasPressed = false;
int presses = 0;
unsigned long pressStart = 0;

void setup() {
  Serial.begin(115200);
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(1500);
  Serial.println("Sensor test ready. Press the sensor.");
}

void loop() {
  // read 20 times and count how many say "pressed", so one bad read doesn't fool it
  int low = 0;
  for (int i = 0; i < 20; i++) {
    if (digitalRead(SENSOR_PIN) == LOW) low++;
    delay(1);
  }
  int percent = low * 5;              // 0 = not pressed, 100 = pressed the whole time
  bool pressed = percent >= 50;

  digitalWrite(LED_BUILTIN, pressed ? HIGH : LOW);

  if (pressed && !wasPressed) {
    presses++;
    pressStart = millis();
    Serial.print("PRESSED  (press #"); Serial.print(presses); Serial.println(")");
  }
  if (!pressed && wasPressed) {
    Serial.print("released after "); Serial.print((millis() - pressStart) / 1000.0, 2); Serial.println(" s");
  }
  wasPressed = pressed;

  Serial.print("sensor:"); Serial.println(percent);   // for the Serial Plotter
  delay(80);
}
