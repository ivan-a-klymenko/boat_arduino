#include <Arduino.h>
#include <AccelStepper.h>

// Пины драйверов A4988
#define STEP_X 4
#define DIR_X  5
#define STEP_Y 6
#define DIR_Y  7

// Реле (активный LOW)
#define RELAY_PIN 8

// Базовые параметры двигателя (оставлены разумные значения)
#define STEPS_PER_REV 200
const float MAX_SPEED = STEPS_PER_REV / 0.5f; // 2 об/с = 400 шаг/с

// === НАСТРОЙКА МАСШТАБА ПОЗИЦИИ → ШАГИ ===
// 1 ед. по POWER/DIRECTION во сколько шагов?
#define X_STEPS_PER_UNIT 1   // шаговик 1 (диапазон 0..30 ед.)
#define Y_STEPS_PER_UNIT 1   // шаговик 2 (диапазон -30..30 ед.)

// Длительность импульса реле
const unsigned long RELAY_PULSE_MS = 5000;

// Два шаговых двигателя
AccelStepper stepperX(AccelStepper::DRIVER, STEP_X, DIR_X); // Stepper 1 (POWER)
AccelStepper stepperY(AccelStepper::DRIVER, STEP_Y, DIR_Y); // Stepper 2 (DIRECTION)

// --- Состояние реле по таймеру ---
bool relayActive = false;
unsigned long relayOffAt = 0;

// Утилита логов: эхо + ACK (для Android, чтобы видеть, что команда получена)
static void ack(const String& recv, const String& info) {
  Serial.print("RECV ");
  Serial.println(recv);      // эхо полной полученной строки
  Serial.print("ACK ");
  Serial.println(info);      // краткое подтверждение
}

static void relayOnFixed() {
  relayActive = true;
  relayOffAt = millis() + RELAY_PULSE_MS;
  digitalWrite(RELAY_PIN, LOW); // активный LOW
  Serial.print("RELAY ON ");
  Serial.println(RELAY_PULSE_MS);
}

static void relayOff() {
  if (!relayActive) return;
  relayActive = false;
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("RELAY OFF");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // по умолчанию выкл

  // Настройка динамики (используем moveTo/run)
  stepperX.setMaxSpeed(MAX_SPEED);
  stepperX.setAcceleration(MAX_SPEED / 2);
  stepperY.setMaxSpeed(MAX_SPEED);
  stepperY.setAcceleration(MAX_SPEED / 2);

  // Нулевая точка: при старте текущая позиция считается 0
  stepperX.setCurrentPosition(0);
  stepperY.setCurrentPosition(0);

  Serial.println("READY");
}

static bool parseIntAfter(const String& upperLine, int startIdx, int& outVal) {
  if (startIdx < 0 || startIdx >= (int)upperLine.length()) {
    outVal = 0;
    return true; // пустое → трактуем как 0
  }
  String tail = upperLine.substring(startIdx);
  tail.trim();
  outVal = tail.toInt(); // допускает "+N" и "-N"
  return true;
}

void loop() {
  // Парсинг команд по USB CDC
  if (Serial.available()) {
    String raw = Serial.readStringUntil('\n'); // сохраним оригинал для эхо
    raw.trim();

    if (raw.length() > 0) {
      String s = raw;
      s.trim();
      s.toUpperCase(); // регистронезависимо

      if (s == "PING") {
        ack(raw, "PONG");
        Serial.println("PONG");
      }
      else if (s.startsWith("POWER")) {
        // Абсолютная позиция (ед.) в диапазоне 0..30 → конвертация в шаги
        int val = 0;
        parseIntAfter(s, 5, val); // символы после "POWER"
        int units = constrain(val, 0, 30);
        long targetSteps = (long)units * (long)X_STEPS_PER_UNIT;
        stepperX.moveTo(targetSteps);
        ack(raw, String("POWER -> pos ") + units + " (steps " + targetSteps + ")");
      }
      else if (s.startsWith("DIRECTION")) {
        // Абсолютная позиция (ед.) в диапазоне -30..30 → конвертация в шаги
        int val = 0;
        parseIntAfter(s, 9, val); // символы после "DIRECTION"
        int units = constrain(val, -30, 30);
        long targetSteps = (long)units * (long)Y_STEPS_PER_UNIT;
        stepperY.moveTo(targetSteps);
        ack(raw, String("DIRECTION -> pos ") + units + " (steps " + targetSteps + ")");
      }
      else if (s == "RELAY") {
        relayOnFixed(); // фиксированная длительность
        ack(raw, "RELAY");
      }
      else {
        // Неизвестная команда
        Serial.print("RECV ");
        Serial.println(raw);
        Serial.println("ERR UNKNOWN");
      }
    }
  }

  // Авто-выключение реле по таймеру
  if (relayActive && millis() >= relayOffAt) {
    relayOff();
  }

  // Движение к целевой позиции (с ускорением)
  stepperX.run();
  stepperY.run();
}
