#include <Servo.h>

Servo s1, s2;

// ===== Servo angles =====
const int S1_CLOSE = 180;
const int S2_CLOSE = 0;
const int S1_OPEN  = 60;
const int S2_OPEN  = 120;

//buzzer pin
const int BUZZER_PIN = 4;


// ===== HC-SR04 pins =====
const int TRIG_PIN = 7;
const int ECHO_PIN = 6;

// ===== LED pin =====
const int LED_PIN = 13;   // change if needed

// ===== Your wave rule (RAW us) =====
const unsigned long WAVE_US = 500;     // echo < 500 => wave
const unsigned long REARM_US = 900;    // echo > 900 => hand away (re-arm)

// ===== Filter junk =====
const unsigned long MIN_VALID_US = 150; // anything below this is noise/helmet reflection

// ===== Prevent double toggle after movement =====
const unsigned long SETTLE_MS = 1500;
unsigned long lastMoveMs = 0;

bool isOpen = false;
bool armed = true;

unsigned long readEchoUS() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  return pulseIn(ECHO_PIN, HIGH, 30000); // 0 = no echo
}

void moveTo(int a1, int a2) {
  s1.write(a1);
  s2.write(a2);
}

void playHeroFanfare() {
  // Original cinematic hero fanfare (Avengers-style vibe, not the actual theme)
  const int notes[] = {
    392, 392, 494, 587,   // G4 G4 B4 D5
    523, 494, 440, 494,   // C5 B4 A4 B4
    587, 659, 587, 523,   // D5 E5 D5 C5
    494, 523, 587, 784    // B4 C5 D5 G5 (big finish)
  };

  const int dur[] = {
    160, 160, 180, 260,
    160, 160, 180, 220,
    180, 220, 180, 180,
    220, 220, 260, 420
  };

  for (int i = 0; i < (int)(sizeof(notes)/sizeof(notes[0])); i++) {
    tone(BUZZER_PIN, notes[i], dur[i]);
    delay(dur[i] + 35);
  }
  noTone(BUZZER_PIN);
}

void setup() {
  Serial.begin(9600);

  s1.attach(9);
  s2.attach(10);

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  moveTo(S1_CLOSE, S2_CLOSE);
  isOpen = false;

  Serial.println("READY: Wave when echo is 150..500us. Far when >900us.");
}

void loop() {
  if (millis() - lastMoveMs < SETTLE_MS) {
    delay(60);
    return;
  }

  unsigned long us = readEchoUS();

  // Print "real" readings only
  if (us == 0) {
    Serial.println("Echo(us): 0 (no echo)");
  } else if (us < MIN_VALID_US) {
    Serial.print("Echo(us): ");
    Serial.print(us);
    Serial.println(" (ignored tiny/noise)");
  } else {
    Serial.print("Echo(us): ");
    Serial.println(us);
  }

  // Ignore noise and 0 for wave detection
  bool valid = (us >= MIN_VALID_US);

  bool waveNow = (valid && us < WAVE_US);       // 150..499
  bool farNow  = (us == 0 || us > REARM_US);    // far or no echo

  if (farNow) armed = true;

  if (armed && waveNow) {
    armed = false;
    lastMoveMs = millis();

    if (!isOpen) {
      Serial.println("OPEN");
      moveTo(S1_OPEN, S2_OPEN);
      digitalWrite(LED_PIN, HIGH);
      playHeroFanfare();
      isOpen = true;
    } else {
      Serial.println("CLOSE");
      moveTo(S1_CLOSE, S2_CLOSE);
      digitalWrite(LED_PIN, LOW);
      isOpen = false;
    }
  }

  delay(60);
}