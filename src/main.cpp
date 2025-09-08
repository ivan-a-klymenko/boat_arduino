#include <Arduino.h>
#include <AccelStepper.h>

// Пины для драйверов A4988 (оставлены как у тебя)
#define STEP_X 4
#define DIR_X  5
#define STEP_Y 6
#define DIR_Y  7

// Реле (активный LOW в твоём коде)
#define RELAY_PIN 8

#define STEPS_PER_REV 200
const float SPEED = STEPS_PER_REV / 0.5;  // 2 об/с

AccelStepper stepperX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper stepperY(AccelStepper::DRIVER, STEP_Y, DIR_Y);

// --- состояние реле по таймеру ---
bool relayActive = false;
unsigned long relayOffAt = 0;
int lastRelayDurationMs = 0;

void relayOn(int durationMs) {
  relayActive = true;
  lastRelayDurationMs = durationMs;
  relayOffAt = millis() + (durationMs > 0 ? durationMs : 5000);
  digitalWrite(RELAY_PIN, LOW);           // включить (активный LOW)
  Serial.print("RELAY ON ");
  Serial.println(lastRelayDurationMs > 0 ? lastRelayDurationMs : 5000);
}

void relayOff() {
  if (!relayActive) return;
  relayActive = false;
  digitalWrite(RELAY_PIN, HIGH);          // выключить
  Serial.println("RELAY OFF");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("READY");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);          // по умолчанию выкл

  stepperX.setMaxSpeed(SPEED);
  stepperX.setAcceleration(SPEED / 2);
  stepperY.setMaxSpeed(SPEED);
  stepperY.setAcceleration(SPEED / 2);
}

static void handleMove(char command, int steps) {
  switch (command) {
    case 'L': stepperX.moveTo(stepperX.currentPosition() + steps); break;
    case 'R': stepperX.moveTo(stepperX.currentPosition() - steps); break;
    case 'T': stepperY.moveTo(stepperY.currentPosition() + steps); break;
    case 'B': stepperY.moveTo(stepperY.currentPosition() - steps); break;
    default:  Serial.println("ERR UNKNOWN MOVE"); return;
  }
  Serial.print("MOVE ");
  Serial.print(command);
  Serial.print(' ');
  Serial.println(steps);
}

void loop() {
  // Пришли данные по USB CDC?
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim(); // убираем CR/LF

    if (line.length() == 0) { /* пусто */ }
    else if (line.charAt(0) == 'F') {
      // Формат: "F <ms>"
      int ms = 0;
      if (line.length() > 1) {
        ms = line.substring(1).toInt(); // допускаем "F 200" или "F200"
      }
      if (ms <= 0) ms = 5000;
      relayOn(ms);
    }
    else if (line.charAt(0) == 'S') {
      relayOff();
    }
    else if (line.length() >= 3 && (line.charAt(0)=='L' || line.charAt(0)=='R' || line.charAt(0)=='T' || line.charAt(0)=='B')) {
      int steps = line.substring(2).toInt();
      handleMove(line.charAt(0), steps);
    }
    else if (line == "PING") {
      Serial.println("PONG");
    }
    else if (line == "LED ON") {
      // при желании можешь задействовать LED_BUILTIN
      Serial.println("OK");
    }
    else if (line == "LED OFF") {
      Serial.println("OK");
    }
    else {
      Serial.println("ERR UNKNOWN");
    }
  }

  // Авто-выключение реле по таймеру
  if (relayActive && millis() >= relayOffAt) {
    relayOff();
  }

  // Движение шаговых
  stepperX.run();
  stepperY.run();
}
