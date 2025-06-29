#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define STEP_PIN     2
#define DIR_PIN      5
#define EN_PIN       8
#define VALVE_PIN    7
#define BTN_SELECT   9
#define BTN_UP       10
#define BTN_DOWN     11
#define BTN_REPEAT   13
#define BUZZER       12

int fillTime = 5;
int count = 4;
int repeatDelay = 10;
float bottleDistanceCM = 10.0;

bool repeatMode = false;
bool settingRepeatTime = false;
bool running = false;

int setupStep = 0;  // 0: start, 1: FillTime, 2: Count, 3: Distance, 4: Start screen
int currentBottle = 0;

enum State { IDLE, FILLING, ROTATING, WAIT_REPEAT };
State currentState = IDLE;

unsigned long stateStart = 0;
int lastCountdown = -1;

String serialBuffer = "";

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  pinMode(VALVE_PIN, OUTPUT);

  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_REPEAT, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  digitalWrite(EN_PIN, LOW);
  digitalWrite(VALVE_PIN, LOW);

  updateLCD();
  Serial.println("System Ready.");
  Serial.println("Commands:");
  Serial.println("SET FILLTIME <seconds>");
  Serial.println("SET COUNT <number>");
  Serial.println("SET DISTANCE <cm>");
  Serial.println("SET REPEATDELAY <seconds>");
  Serial.println("START");
  Serial.println("STOP");
  Serial.println("TOGGLE REPEAT");
}

void loop() {
  handleSerialInput();
  handleButtons();
  if (running) runStateMachine();
}

void handleSerialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuffer.length() > 0) {
        processCommand(serialBuffer);
        serialBuffer = "";
      }
    } else serialBuffer += c;
  }
}

void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd.startsWith("SET FILLTIME ")) {
    int val = cmd.substring(13).toInt();
    if (val >= 1 && val <= 60) fillTime = val;
  } else if (cmd.startsWith("SET COUNT ")) {
    int val = cmd.substring(10).toInt();
    if (val >= 1 && val <= 20) count = val;
  } else if (cmd.startsWith("SET REPEATDELAY ")) {
    int val = cmd.substring(16).toInt();
    if (val >= 1 && val <= 300) repeatDelay = val;
  } else if (cmd.startsWith("SET DISTANCE ")) {
    float val = cmd.substring(13).toFloat();
    if (val > 0) bottleDistanceCM = val;
  } else if (cmd == "START") {
    if (!running) {
      running = true;
      currentBottle = 0;
      currentState = FILLING;
      stateStart = millis();
    }
  } else if (cmd == "STOP") {
    running = false;
    repeatMode = false;
    currentState = IDLE;
    digitalWrite(VALVE_PIN, LOW);
    updateLCD();
  } else if (cmd == "TOGGLE REPEAT") {
    repeatMode = !repeatMode;
    updateLCD();
  }
  updateLCD();
}

void handleButtons() {
  static bool lastRepeat = HIGH;
  bool nowRepeat = digitalRead(BTN_REPEAT);
  if (running && lastRepeat == HIGH && nowRepeat == LOW) {
    running = false;
    repeatMode = false;
    currentState = IDLE;
    digitalWrite(VALVE_PIN, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Stopped");
    tone(BUZZER, 1000); delay(800); noTone(BUZZER);
    delay(1000);
    setupStep = 0;
    updateLCD();
  }
  lastRepeat = nowRepeat;

  if (!running && !settingRepeatTime) {
    if (digitalRead(BTN_SELECT) == LOW) {
      setupStep++;
      if (setupStep > 4) setupStep = 4;
      tone(BUZZER, 1000); delay(100); noTone(BUZZER);
      delay(250);
      updateLCD();
    }
    if (digitalRead(BTN_UP) == LOW) {
      if (setupStep == 1) fillTime++;
      else if (setupStep == 2) count++;
      else if (setupStep == 3) bottleDistanceCM++;
      tone(BUZZER, 1200); delay(80); noTone(BUZZER);
      delay(200);
      updateLCD();
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      if (setupStep == 1 && fillTime > 1) fillTime--;
      else if (setupStep == 2 && count > 1) count--;
      else if (setupStep == 3 && bottleDistanceCM > 1) bottleDistanceCM--;
      tone(BUZZER, 800); delay(80); noTone(BUZZER);
      delay(200);
      updateLCD();
    }
    if (setupStep == 4 && digitalRead(BTN_SELECT) == LOW) {
      tone(BUZZER, 1500); delay(150); noTone(BUZZER);
      delay(200);
      lcd.clear();
      lcd.print("Starting...");
      delay(500);
      currentBottle = 0;
      stateStart = millis();
      currentState = FILLING;
      running = true;
    }
    if (digitalRead(BTN_REPEAT) == LOW) {
      settingRepeatTime = true;
      tone(BUZZER, 1100); delay(100); noTone(BUZZER);
      delay(250);
      updateLCD();
    }
  }

  if (settingRepeatTime) {
    if (digitalRead(BTN_UP) == LOW) {
      repeatDelay++;
      tone(BUZZER, 1000); delay(50); noTone(BUZZER);
      delay(200);
      updateLCD();
    }
    if (digitalRead(BTN_DOWN) == LOW) {
      if (repeatDelay > 1) repeatDelay--;
      tone(BUZZER, 900); delay(50); noTone(BUZZER);
      delay(200);
      updateLCD();
    }
    if (digitalRead(BTN_REPEAT) == LOW) {
      repeatMode = !repeatMode;
      settingRepeatTime = false;
      tone(BUZZER, 1400); delay(100); noTone(BUZZER);
      delay(300);
      updateLCD();
    }
  }
}

void runStateMachine() {
  unsigned long now = millis();
  switch (currentState) {
    case FILLING:
      digitalWrite(VALVE_PIN, HIGH);
      lcd.setCursor(0, 0);
      lcd.print("Bottle: " + String(currentBottle + 1) + "   ");
      lcd.setCursor(0, 1);
      lcd.print("Filling...     ");
      if (now - stateStart >= (unsigned long)fillTime * 1000) {
        digitalWrite(VALVE_PIN, LOW);
        if (currentBottle < count - 1) {
          currentState = ROTATING;
          stateStart = millis();
          // Show rotating message immediately
         
          lcd.setCursor(0, 1);
          lcd.print("Rotating Bottle");
         
          rotateToNextBottle();
        } else {
          if (repeatMode) {
            currentState = WAIT_REPEAT;
            stateStart = millis();
            lastCountdown = -1;
            tone(BUZZER, 2000); delay(300); noTone(BUZZER);
          } else {
            tone(BUZZER, 2000); delay(1000); noTone(BUZZER);
            running = false;
            setupStep = 0;
            updateLCD();
          }
        }
      }
      break;

    case ROTATING:
      // wait for rotation duration (assuming ~1 second)
      if (now - stateStart >= 1000) {
        currentBottle++;
        stateStart = millis();
        currentState = FILLING;
      }
      break;

    case WAIT_REPEAT:
      {
        unsigned long passed = (millis() - stateStart);
        int remain = repeatDelay - (passed / 1000);
        if (remain != lastCountdown) {
          lastCountdown = remain;
          lcd.setCursor(0, 0);
          lcd.print("Next round in:");
          lcd.setCursor(0, 1);
          lcd.print(String(remain) + " sec       ");
        }
        if (remain <= 0) {
          currentBottle = 0;
          stateStart = millis();
          currentState = FILLING;
          tone(BUZZER, 1800); delay(300); noTone(BUZZER);
        }
      }
      break;
  }
}

void updateLCD() {
  lcd.clear();
  if (settingRepeatTime) {
    lcd.setCursor(0, 0); lcd.print("Set Repeat Time ");
    lcd.setCursor(0, 1); lcd.print(String(repeatDelay) + " sec       ");
    return;
  }
  if (setupStep == 0) {
    lcd.setCursor(0, 0); lcd.print("Press SELECT");
    lcd.setCursor(0, 1); lcd.print("to setup");
  } else if (setupStep == 1) {
    lcd.setCursor(0, 0); lcd.print("Set Fill Time:");
    lcd.setCursor(0, 1); lcd.print(String(fillTime) + " sec");
  } else if (setupStep == 2) {
    lcd.setCursor(0, 0); lcd.print("Set Bottle Count");
    lcd.setCursor(0, 1); lcd.print(String(count));
  } else if (setupStep == 3) {
    lcd.setCursor(0, 0); lcd.print("Set Distance:");
    lcd.setCursor(0, 1); lcd.print(String(bottleDistanceCM, 1) + " cm");
  } else if (setupStep == 4) {
    lcd.setCursor(0, 0); lcd.print("Setup Complete");
    lcd.setCursor(0, 1); lcd.print("Press START");
  }
  lcd.setCursor(15, 0);
  lcd.print(repeatMode ? "R" : "-");
}

void rotateToNextBottle() {
  float stepsPerRev = 200.0;
  float microsteps = 16.0;
  float mmPerCM = 10.0;
  float mmPerRev = 360.0;
  float distancePerStep = mmPerRev / (stepsPerRev * microsteps);
  int stepsNeeded = (int)(bottleDistanceCM * mmPerCM / distancePerStep);

  digitalWrite(DIR_PIN, HIGH);
  for (int i = 0; i < stepsNeeded; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(700);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(700);
  }
}
