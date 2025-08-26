#include <Arduino.h>
#include <SoftwareSerial.h>
#include <AccelStepper.h>

// Bluetooth HC-05
#define BT_RX 2
#define BT_TX 3
SoftwareSerial BTSerial(BT_RX, BT_TX);

// Пины для драйверов A4988
#define STEP_X 4
#define DIR_X 5
#define STEP_Y 6
#define DIR_Y 7

// Пин для управления реле
#define RELAY_PIN 8

#define STEPS_PER_REV 200
const float SPEED = STEPS_PER_REV / 0.5;  // 2 оборот = 1 секунд

// Создаем два объекта AccelStepper (в режиме DRIVER)
AccelStepper stepperX(AccelStepper::DRIVER, STEP_X, DIR_X);
AccelStepper stepperY(AccelStepper::DRIVER, STEP_Y, DIR_Y);

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);

  // Настройка пина для реле
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Реле выключено по умолчанию

  // Настройка шаговых двигателей
  stepperX.setMaxSpeed(SPEED);
  stepperX.setAcceleration(SPEED / 2);  // Ускорение в 2 раза меньше скорости

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
  if (command == 'L') stepperX.moveTo(stepperX.currentPosition() + steps);
  if (command == 'R') stepperX.moveTo(stepperX.currentPosition() - steps);
  if (command == 'T') stepperY.moveTo(stepperY.currentPosition() + steps);
  if (command == 'B') stepperY.moveTo(stepperY.currentPosition() - steps);
  if (command == 'F') activateRelay();
  if (command == 'S') deactivateRelay();

  String message = "MOVE " + String(command) + " " + String(steps);
  Serial.println(message);
  BTSerial.println(message);
}

void loop() {
  // Читаем команды из Bluetooth
  if (BTSerial.available() > 0) {
    String command = BTSerial.readStringUntil('\n');
    command.trim();

    char direction = command.charAt(0);
    int steps = command.substring(2).toInt();

    handleCommand(direction, steps);
  }

  // Двигаем двигатели
  stepperX.run();
  stepperY.run();
}
