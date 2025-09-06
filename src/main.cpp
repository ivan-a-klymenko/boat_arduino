#include <Arduino.h>
#include <AccelStepper.h>

// Пины для драйверов A4988
#define STEP_X 4
#define DIR_X 5
#define STEP_Y 6
#define DIR_Y 7

// Пин для управления реле
#define RELAY_PIN 8

#define STEPS_PER_REV 200
const float SPEED = STEPS_PER_REV / 0.5;  // 2 оборота = 1 секунда

// Два шаговых мотора
AccelStepper stepperX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper stepperY(AccelStepper::DRIVER, STEP_Y, DIR_Y);

void setup() {
  // Запускаем USB CDC (Serial)
  Serial.begin(115200);
  while (!Serial) {
    delay(10);  // Ждем готовности USB CDC
  }
  Serial.println("ESP32-S3 USB CDC готов к приёму команд");

  // Настройка пина для реле
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Реле выключено по умолчанию

  // Настройка шаговых двигателей
  stepperX.setMaxSpeed(SPEED);
  stepperX.setAcceleration(SPEED / 2);

  stepperY.setMaxSpeed(SPEED);
  stepperY.setAcceleration(SPEED / 2);
}

void activateRelay() {
  Serial.println("Реле включено");
  digitalWrite(RELAY_PIN, LOW);
}

void deactivateRelay() {
  digitalWrite(RELAY_PIN, HIGH);
  Serial.println("Реле выключено");
}

void handleCommand(char command, int steps) {
  switch (command) {
    case 'L': stepperX.moveTo(stepperX.currentPosition() + steps); break;
    case 'R': stepperX.moveTo(stepperX.currentPosition() - steps); break;
    case 'T': stepperY.moveTo(stepperY.currentPosition() + steps); break;
    case 'B': stepperY.moveTo(stepperY.currentPosition() - steps); break;
    case 'F': activateRelay(); break;
    case 'S': deactivateRelay(); break;
    default:
      Serial.println("Неизвестная команда");
      return;
  }

  String message = "MOVE " + String(command) + " " + String(steps);
  Serial.println(message);  // Ответ по USB
}

void loop() {
  // Чтение команд через USB (Serial)
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() < 2) return;

    char direction = command.charAt(0);
    int steps = command.substring(2).toInt();

    handleCommand(direction, steps);
  }

  // Обработка шагов моторов
  stepperX.run();
  stepperY.run();
}
