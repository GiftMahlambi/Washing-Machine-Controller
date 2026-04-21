
#include <IRremote.hpp>

// Pin definitions
const int DRUM_LED_1 = 4;
const int DRUM_LED_2 = 5;
const int DRUM_LED_3 = 6;
const int DRUM_LED_4 = 7;
const int VALVE_LED = 8;
const int PUMP_MOTOR = 9;   // Pump motor

// Input pins
const int START_BUTTON   = 10;   // Button
const int WATER_LEVEL_PIN = A5;  // Potentiometer

// IR Receiver
const int IR_RECEIVE_PIN = 3;

// Drum LED array
const int drumLeds[4] = {DRUM_LED_1, DRUM_LED_2, DRUM_LED_3, DRUM_LED_4};

// System variables
bool machineRunning = false;
bool buttonPressed = false;
int currentCycle = 0;
int lastCycle = -1;
int powerLevel = 0;
int waterLevel = 0;
bool drumDirection = true;
unsigned long cycleTimer = 0;
bool waitingToStart = true;

// Cycle definitions
enum WashCycles {
  IDLE = 0,
  FILLING = 1,
  WASHING = 2,
  DRAINING1 = 3,
  REFILLING = 4,
  RINSING = 5,
  DRAINING2 = 6,
  SPINNING = 7,
  COMPLETE = 8
};
//In order to move forward each cylcle do this
bool waitForSpinPower(unsigned long timeoutMs);
void handleIR(uint32_t code);
void stopAllMotors();
void nextCycle(int newCycle);

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < 4; i++) pinMode(drumLeds[i], OUTPUT);
  pinMode(VALVE_LED, OUTPUT);
  pinMode(PUMP_MOTOR, OUTPUT);
  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(WATER_LEVEL_PIN, INPUT);

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  stopAllMotors();
  digitalWrite(VALVE_LED, LOW);
  analogWrite(PUMP_MOTOR, 0);

  Serial.println("Solar Washing Machine Ready");
  Serial.println("Use IR remote to set power category (0=No Power,1=Low,2=Medium,3=High).");
}

void loop() {
  if (IrReceiver.decode()) {
    handleIR(IrReceiver.decodedIRData.decodedRawData);
    IrReceiver.resume();
  }

  if (waitingToStart && powerLevel >= 2) {
    Serial.println("Press START button to start machine");
    waitingToStart = false;
  }

  if (digitalRead(START_BUTTON) == LOW && !buttonPressed) {
    buttonPressed = true;
    delay(50);
    if (!machineRunning) {
      if (powerLevel >= 2) {
        machineRunning = true;
        currentCycle = FILLING;
        cycleTimer = millis();
        Serial.println("Starting wash cycle...");
        lastCycle = -1;
      } else {
        Serial.println("Not enough power to start cycle! Minimum: Category 2 (Medium Power)");
      }
    } else {
      machineRunning = false;
      stopAllMotors();
      digitalWrite(VALVE_LED, LOW);
      analogWrite(PUMP_MOTOR, 0);
      Serial.println("Machine paused by user");
    }
  } else if (digitalRead(START_BUTTON) == HIGH) {
    buttonPressed = false;
  }

  if (machineRunning) runWashCycle();

  delay(100);
}

// Cycle Handling 

void checkPower(int requiredPower) {
  if (powerLevel < requiredPower) {
    Serial.print("Not enough power! Paused. Minimum needed: ");
    Serial.println(requiredPower);
    stopAllMotors();
    digitalWrite(VALVE_LED, LOW);
    analogWrite(PUMP_MOTOR, 0);

    unsigned long waitStart = millis();
    while (powerLevel < requiredPower) {
      if (IrReceiver.decode()) {
        handleIR(IrReceiver.decodedIRData.decodedRawData);
        IrReceiver.resume();
      }
      if (millis() - waitStart > 15000) break; // Prevent infinite wait
      delay(200);
    }
    Serial.println("Power restored. Resuming cycle...");
    cycleTimer = millis();
  }
}

void runWashCycle() {
  unsigned long currentTime = millis();

  if (currentCycle != lastCycle) {
    switch (currentCycle) {
      case FILLING: Serial.println("Cycle 1: FILLING"); break;
      case WASHING: Serial.println("Cycle 2: WASHING"); break;
      case DRAINING1: Serial.println("Cycle 3: DRAINING"); break;
      case REFILLING: Serial.println("Cycle 4: REFILLING"); break;
      case RINSING: Serial.println("Cycle 5: RINSING"); break;
      case DRAINING2: Serial.println("Cycle 6: DRAINING"); break;
      case SPINNING: Serial.println("Cycle 7: SPINNING"); break;
      case COMPLETE: Serial.println("Cycle 8: COMPLETE"); break;
      default: break;
    }
    lastCycle = currentCycle;
  }

  switch (currentCycle) {
    case FILLING:
      checkPower(2);
      fillDrum();
      if (waterLevel >= 80 || millis() - cycleTimer > 10000) {
        digitalWrite(VALVE_LED, LOW);
        nextCycle(WASHING);
      }
      break;

    case WASHING:
      checkPower(2);
      wash_rinse();
      if (currentTime - cycleTimer > 15000) nextCycle(DRAINING1);
      break;

    case DRAINING1:
      checkPower(2);
      draining();
      if (currentTime - cycleTimer > 5000) nextCycle(REFILLING);
      break;

    case REFILLING:
      checkPower(2);
      fillDrum();
      if (waterLevel >= 80 || millis() - cycleTimer > 10000) {
        digitalWrite(VALVE_LED, LOW);
        nextCycle(RINSING);
      }
      break;

    case RINSING:
      checkPower(2);
      rinsing();
      if (currentTime - cycleTimer > 10000) nextCycle(DRAINING2);
      break;

    case DRAINING2:
      checkPower(2);
      draining();
      if (currentTime - cycleTimer > 5000) {
        Serial.println("Waiting for IR input to start SPINNING...");
        bool gotSpinPower = waitForSpinPower(20000);
        if (!gotSpinPower) Serial.println("No spin power within timeout. Proceeding for testing.");
        nextCycle(SPINNING);
      }
      break;

    case SPINNING:
      checkPower(2);
      spinning();
      if (currentTime - cycleTimer > 8000) nextCycle(COMPLETE);
      break;

    case COMPLETE:
      completeCycle();
      break;
  }
}

// Cycle functions

void fillDrum() {
  waterLevel = readWaterLevel();
  if (waterLevel < 80) digitalWrite(VALVE_LED, HIGH);
  else {
    digitalWrite(VALVE_LED, LOW);
    waterLevel = 80;
  }

  analogWrite(PUMP_MOTOR, 0);

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000) {
    Serial.print("Filling... Water level: ");
    Serial.print(waterLevel);
    Serial.print("% | ");
    Serial.println(getPowerDescription());
    lastMsg = millis();
  }

  rotateDrum(getDrumSpeed());
}

void wash_rinse() {
  analogWrite(PUMP_MOTOR, 0);
  static unsigned long directionTimer = millis();
  if (millis() - directionTimer > 3000) {
    drumDirection = !drumDirection;
    directionTimer = millis();
    Serial.println("Drum direction changed");
  }
  rotateDrum(getDrumSpeed());

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000) {
    Serial.print("Washing... ");
    Serial.println(getPowerDescription());
    lastMsg = millis();
  }
}

void draining() {
  analogWrite(PUMP_MOTOR, getPumpSpeed());
  stopAllMotors();
  unsigned long elapsed = millis() - cycleTimer;
  waterLevel = map(elapsed, 0, 5000, 100, 0);
  waterLevel = constrain(waterLevel, 0, 100);

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000) {
    Serial.print("Draining... Water level: ");
    Serial.print(waterLevel);
    Serial.print("% | ");
    Serial.println(getPowerDescription());
    lastMsg = millis();
  }
  delay(10);
}

void rinsing() {
  static bool fillingState = true;
  if (fillingState && (millis() - cycleTimer < 4000)) {
    digitalWrite(VALVE_LED, HIGH);
    analogWrite(PUMP_MOTOR, 0);
  } else {
    fillingState = false;
    digitalWrite(VALVE_LED, LOW);
    rotateDrum(getDrumSpeed());
  }

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000) {
    Serial.print("Rinsing... ");
    Serial.println(getPowerDescription());
    lastMsg = millis();
  }
}

void spinning() {
  analogWrite(PUMP_MOTOR, 0);
  digitalWrite(VALVE_LED, LOW);

  int stepIntervalMs;
  if (powerLevel == 3) stepIntervalMs = 40;
  else if (powerLevel == 2) stepIntervalMs = 140;
  else stepIntervalMs = 200;

  static unsigned long spinSessionKey = 0;
  static unsigned long lastStepTime = 0;
  static int stepCount = 0;

  if (spinSessionKey != cycleTimer) {
    spinSessionKey = cycleTimer;
    lastStepTime = 0;
    stepCount = 0;
    Serial.println("Starting spin: target = 20 full revolutions...");
  }

  unsigned long now = millis();
  if (lastStepTime == 0 || (now - lastStepTime) >= (unsigned long)stepIntervalMs) {
    rotateDrum(1);
    lastStepTime = now;
    stepCount++;
    if (stepCount >= 80) {
      Serial.println("20 full revolutions complete.");
      nextCycle(COMPLETE);
      return;
    }
  }

  static unsigned long lastMsg = 0;
  if (now - lastMsg > 1000) {
    Serial.print("Spinning... | Revolutions: ");
    Serial.print(stepCount / 4);
    Serial.print("/20 | ");
    Serial.println(getPowerDescription());
    lastMsg = now;
  }
}

void completeCycle() {
  stopAllMotors();
  machineRunning = false;
  currentCycle = IDLE;
  analogWrite(PUMP_MOTOR, 0);
  digitalWrite(VALVE_LED, LOW);
  powerLevel = 0;
  waitingToStart = true;
  Serial.println("Wash cycle complete. Please select power again and press START for next load.");
}

// Utility Functions 

int getDrumSpeed() {
  switch (powerLevel) {
    case 2: return 450;
    case 3: return 220;
    default: return 600;
  }
}

int getPumpSpeed() {
  switch (powerLevel) {
    case 1: return 85;
    case 2: return 170;
    case 3: return 255;
    default: return 0;
  }
}

String getPowerDescription() {
  switch (powerLevel) {
    case 0: return "Power 0: No Power";
    case 1: return "Power 1: Low Power";
    case 2: return "Power 2: Medium Power";
    case 3: return "Power 3: High Power";
    default: return "Unknown Power";
  }
}

void rotateDrum(int rotationDelay) {
  static unsigned long lastRotation = 0;
  static int ledPattern = 0;

  if (millis() - lastRotation > rotationDelay) {
    for (int i = 0; i < 4; i++) digitalWrite(drumLeds[i], LOW);

    if (drumDirection) {
      ledPattern++;
      if (ledPattern > 3) ledPattern = 0;
    } else {
      ledPattern--;
      if (ledPattern < 0) ledPattern = 3;
    }

    digitalWrite(drumLeds[ledPattern], HIGH);
    lastRotation = millis();
  }
}

void stopAllMotors() {
  for (int i = 0; i < 4; i++) digitalWrite(drumLeds[i], LOW);
}

int readWaterLevel() {
  int sensorValue = analogRead(WATER_LEVEL_PIN);
  int level = map(sensorValue, 0, 1023, 0, 100);
  return constrain(level, 0, 100);
}

bool waitForSpinPower(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (true) {
    if (IrReceiver.decode()) {
      handleIR(IrReceiver.decodedIRData.decodedRawData);
      IrReceiver.resume();
      if (powerLevel >= 2) {
        Serial.println("Spin power accepted.");
        return true;
      } else {
        Serial.println("Not enough power for spinning. Waiting...");
      }
    }
    if (timeoutMs > 0 && (millis() - start >= timeoutMs)) return false;
    delay(200);
  }
}

void handleIR(uint32_t code) {
  Serial.print("IR code received: ");
  Serial.println(code, HEX);

  if (code == 0xF30CBF00) { powerLevel = 0; Serial.println("Power Category 0: No Power"); }
  if (code == 0xEF10BF00) { powerLevel = 1; Serial.println("Power Category 1: Low Power"); }
  if (code == 0xEE11BF00) { powerLevel = 2; Serial.println("Power Category 2: Medium Power"); }
  if (code == 0xED12BF00) { powerLevel = 3; Serial.println("Power Category 3: High Power"); }
}

void nextCycle(int newCycle) {
  currentCycle = newCycle;
  cycleTimer = millis();
  lastCycle = -1;
}
