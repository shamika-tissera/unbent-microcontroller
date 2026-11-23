/*
  Unbent bench test (Arduino Nano 33 BLE Sense Rev2):
  pressure sensor under the heel -> stepper tightens the reel -> servo lifts the shoehorn.

  WIRING
    Pressure sensor (FSR), NO resistor (HAVE_RESISTOR = false, the default):
                           one leg -> A0, other leg -> GND. The Nano's built-in pull-up does the resistor's job.
                           It only says pressed / not pressed, which is all the cycle needs. Press firmly.
    Pressure sensor (FSR), WITH a 10k resistor (set HAVE_RESISTOR = true): gives a 0-1023 force reading.
                           one leg -> 3.3V, other leg -> A0, and the 10k from A0 -> GND.
                           (your Nano 33 BLE is a 3.3 V board: never 5V on A0)
    Servo                  orange -> D9   red -> 5V   brown -> GND
    ULN2003 driver board   IN1 -> D4  IN2 -> D5  IN3 -> D6  IN4 -> D7
                           +  -> 5V   -  -> GND
    Stepper                white plug into the ULN2003 board
    5V: the Nano's 5V pin (USB powered) is fine for testing ONE motor at a time.
    For full cycles (c) use a separate 5V supply (USB power bank) for the servo and ULN2003,
    and join its GND to the Nano's GND. The Nano itself runs from the laptop USB.

  SETUP (Nano 33 BLE Sense Rev2): Arduino IDE > Boards Manager > install "Arduino Mbed OS Nano Boards",
  then Tools > Board > Arduino Nano 33 BLE. If upload fails, double-press the white reset button and upload again.

  USE: open Serial Monitor at 115200 baud, line ending "Newline", then type:
    k        calibrate (empty shoe, then foot in)
    r        show the sensor reading for 10 s
    s1500    move servo to 1500 us (try 500..2500 to find DOWN and UP)
    d1500    set the DOWN value      u2300   set the UP value
    t0.5     stepper turns half a turn (tighten). t-0.5 = other way
    c10      run 10 full cycles. Lines starting with CSV are your test data
*/
#include <Servo.h>

const int FSR_PIN = A0;
const bool HAVE_RESISTOR = false;     // no 10k resistor yet: use the built-in pull-up
const int SERVO_PIN = 9;
const int COIL[4] = {4, 5, 6, 7};

int servoDown = 1500;
int servoUp = 2300;
float tightenTurns = 0.75;
int direction = 1;                       // if the reel does not click when tightening, change to -1
const long HALF_STEPS_PER_TURN = 4096;
const unsigned int STEP_US = 2000;       // bigger = slower but stronger
const unsigned long HOLD_MS = 1500;      // heel pressed this long = foot is in
const unsigned long CLEAR_MS = 2000;     // released this long = foot is out
const unsigned long TIMEOUT_MS = 60000;

const uint8_t SEQ[8][4] = {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
                           {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}};
Servo servo;
int phase = 0;
int baseline = -1, footLevel = -1, threshold = -1;

int readFsr() {
  long s = 0;
  if (HAVE_RESISTOR) {
    for (int i = 0; i < 16; i++) s += analogRead(FSR_PIN);
    return (int)(s / 16);
  }
  // built-in pull-up: pin reads LOW when the sensor is pressed. 16 samples -> 0 (free) .. 1000 (pressed)
  for (int i = 0; i < 16; i++) { if (digitalRead(FSR_PIN) == LOW) s++; delayMicroseconds(200); }
  return (int)(s * 1000 / 16);
}

void servoTo(int us) {
  servo.attach(SERVO_PIN);
  servo.writeMicroseconds(constrain(us, 500, 2500));
  delay(700);
  servo.detach();                        // stop pulses so it does not buzz
}

unsigned long turnReel(float turns) {
  long n = (long)(fabs(turns) * HALF_STEPS_PER_TURN);
  int d = (turns >= 0) ? direction : -direction;
  unsigned long t0 = millis();
  for (long k = 0; k < n; k++) {
    phase = (phase + d + 8) % 8;
    for (int c = 0; c < 4; c++) digitalWrite(COIL[c], SEQ[phase][c]);
    delayMicroseconds(STEP_US);
  }
  for (int c = 0; c < 4; c++) digitalWrite(COIL[c], LOW);   // coils off: the ratchet holds the lace
  return millis() - t0;
}

bool footIn() {
  bool on = readFsr() > threshold;
  digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
  return on;
}

// wait until the foot is in (wantFoot = true) or out (false) for holdMs. Returns ms taken, or 0 on timeout.
unsigned long waitFoot(bool wantFoot, unsigned long holdMs) {
  unsigned long t0 = millis(), since = 0;
  while (millis() - t0 < TIMEOUT_MS) {
    if (footIn() == wantFoot) {
      if (since == 0) since = millis();
      if (millis() - since >= holdMs) return millis() - t0;
    } else {
      since = 0;
    }
    delay(20);
  }
  return 0;
}

void waitEnter(const char *msg) {
  Serial.println(msg);
  while (Serial.available()) Serial.read();
  while (!Serial.available()) delay(10);
  while (Serial.available()) { Serial.read(); delay(2); }
}

void calibrate() {
  waitEnter("Put the EMPTY shoe on the sensor, then press Enter.");
  baseline = readFsr();
  waitEnter("Now put your foot in and press down with your heel, then press Enter.");
  footLevel = readFsr();
  threshold = baseline + (footLevel - baseline) / 2;
  Serial.print("empty="); Serial.print(baseline);
  Serial.print("  foot="); Serial.print(footLevel);
  Serial.print("  threshold="); Serial.println(threshold);
  if (footLevel - baseline < 40) {
    Serial.println("WARNING: small difference. Glue a coin on the sensor so the heel presses one spot, press harder, check wiring.");
  }
  waitEnter("Take your foot out, then press Enter.");
}

void runCycles(int n) {
  if (threshold < 0) calibrate();
  servoTo(servoDown);
  Serial.println("CSV,cycle,foot_detected,seconds_to_detect,tighten_seconds,foot_left");
  for (int k = 1; k <= n; k++) {
    Serial.print("Cycle "); Serial.print(k); Serial.println(": put your foot in.");
    unsigned long tIn = waitFoot(true, HOLD_MS);
    if (tIn == 0) {
      Serial.print("CSV,"); Serial.print(k); Serial.println(",false,,,");
      continue;
    }
    Serial.println("  FOOT IN -> tightening");
    unsigned long tTight = turnReel(tightenTurns);
    Serial.println("  lifting the shoehorn, step out");
    servoTo(servoUp);
    unsigned long tOut = waitFoot(false, CLEAR_MS);
    servoTo(servoDown);
    Serial.print("CSV,"); Serial.print(k); Serial.print(",true,");
    Serial.print(tIn / 1000.0, 2); Serial.print(",");
    Serial.print(tTight / 1000.0, 2); Serial.print(",");
    Serial.println(tOut > 0 ? "true" : "false");
  }
  Serial.println("Done. Copy the CSV lines into a text file.");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  if (!HAVE_RESISTOR) pinMode(FSR_PIN, INPUT_PULLUP);
  for (int c = 0; c < 4; c++) { pinMode(COIL[c], OUTPUT); digitalWrite(COIL[c], LOW); }
  delay(1500);
  Serial.println("Unbent bench test ready. Commands: k r s d u t c (see top of the sketch)");
}

void loop() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;
  char cmd = line.charAt(0);
  float val = line.substring(1).toFloat();
  switch (cmd) {
    case 'k': calibrate(); break;
    case 'r': {
      unsigned long t0 = millis();
      while (millis() - t0 < 10000) { Serial.println(readFsr()); delay(100); }
      break;
    }
    case 's': servoTo((int)val); Serial.print("servo "); Serial.println((int)val); break;
    case 'd': servoDown = (int)val; Serial.print("DOWN = "); Serial.println(servoDown); break;
    case 'u': servoUp = (int)val; Serial.print("UP = "); Serial.println(servoUp); break;
    case 't': Serial.print("took "); Serial.print(turnReel(val) / 1000.0, 1); Serial.println(" s"); break;
    case 'c': runCycles(val > 0 ? (int)val : 5); break;
    default: Serial.println("unknown command");
  }
}
