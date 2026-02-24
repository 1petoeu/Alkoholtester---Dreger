#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MQ3_PIN A0
#define BATTERY_PIN A1
#define START_BTN 2
#define CALIB_BTN 5
#define BUZZER_PIN 6

float alcohol_promile = 0;
int baseline = 100;

bool lastStartState = HIGH;
bool lastCalibState = HIGH;
bool sensorWarmedUp = false;

void setup() {
  pinMode(START_BTN, INPUT_PULLUP);
  pinMode(CALIB_BTN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);

  bootScreen();
  warmupSensor(); // 30s warmup pri starte
}

void loop() {
  handleCalibButton();
  handleStartButton();

  static unsigned long last_update = 0;
  if (millis() - last_update > 300) {
    updateDisplay();
    last_update = millis();
  }

  delay(10);
}

// ── Warmup 30 sekund ────────────────────────────────────────────────────────
void warmupSensor() {
  int totalSeconds = 30;

  for (int s = totalSeconds; s >= 1; s--) {
    // Skontroluje či bolo stlačené 3x kalibračné tlačidlo
    static int calibPresses = 0;
    bool state = digitalRead(CALIB_BTN);
    if (lastCalibState == HIGH && state == LOW) {
      calibPresses++;
      tone(BUZZER_PIN, 1500, 80);
      if (calibPresses >= 3) {
        calibPresses = 0;
        sensorWarmedUp = true;
        tone(BUZZER_PIN, 1800, 200);
        delay(200);
        tone(BUZZER_PIN, 1800, 200);
        return;
      }
    }
    lastCalibState = state;

    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(18, 2);
    display.print("OHREV SENZORA...");

    int barWidth = (int)((float)(totalSeconds - s) / totalSeconds * 120.0);
    display.drawRect(4, 14, 120, 8, SSD1306_WHITE);
    display.fillRect(4, 14, barWidth, 8, SSD1306_WHITE);

    display.setTextSize(4);
    int xPos = (s >= 10) ? 40 : 52;
    display.setCursor(xPos, 28);
    display.print(s);

    display.setTextSize(1);
    display.setCursor(46, 56);
    display.print("sekund");

    display.display();

    // Čakaj 1 sekundu ale kontroluj tlačidlo každých 10ms
    for (int ms = 0; ms < 100; ms++) {
      state = digitalRead(CALIB_BTN);
      if (lastCalibState == HIGH && state == LOW) {
        calibPresses++;
        tone(BUZZER_PIN, 1500, 80);
        if (calibPresses >= 3) {
          calibPresses = 0;
          sensorWarmedUp = true;
          tone(BUZZER_PIN, 1800, 200);
          delay(200);
          tone(BUZZER_PIN, 1800, 200);
          return;
        }
      }
      lastCalibState = state;
      delay(10);
    }
  }

  sensorWarmedUp = true;
  tone(BUZZER_PIN, 1800, 200);
  delay(200);
  tone(BUZZER_PIN, 1800, 200);
}

// ── Kalibrácia ──────────────────────────────────────────────────────────────
void handleCalibButton() {
  bool state = digitalRead(CALIB_BTN);

  if (lastCalibState == HIGH && state == LOW) {
    calibrate();
    delay(200);
  }

  lastCalibState = state;
}

void calibrate() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(20, 10);
  display.println("KALIBRACIA...");
  display.display();

  long sum = 0;
  for (int i = 0; i < 20; i++) {
    sum += analogRead(MQ3_PIN);
    delay(50);
  }
  baseline = sum / 20;

  tone(BUZZER_PIN, 1500, 100);
  delay(150);
  tone(BUZZER_PIN, 1500, 100);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(25, 10);
  display.println("HOTOVO!");
  display.setCursor(10, 28);
  display.print("Baseline: ");
  display.print(baseline);
  display.display();
  delay(1500);
}

// ── Start button ─────────────────────────────────────────────────────────────
void handleStartButton() {
  bool state = digitalRead(START_BTN);

  if (lastStartState == HIGH && state == LOW) {
    tone(BUZZER_PIN, 1200, 150);
    measureAlcohol();
    delay(200);
  }

  lastStartState = state;
}

void measureAlcohol() {
  // Po warme len 3 sekundy odpočet
 int countdown = sensorWarmedUp ? 5 : 10;

  for (int i = countdown; i >= 1; i--) {
    display.clearDisplay();

    display.setTextSize(2);
    display.setCursor(24, 2);
    display.print("FUKAJ!"); // FÚKAJ

    display.setTextSize(4);
    int xPos = 52;
    display.setCursor(xPos, 24);
    display.print(i);

    display.display();
    tone(BUZZER_PIN, 1000, 150);
    delay(1000);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(18, 24);
  display.println("MERANIE...");
  display.display();
  delay(800);

  int max_val = 0;
  for (int i = 0; i < 10; i++) {
    int val = analogRead(MQ3_PIN);
    if (val > max_val) max_val = val;
    delay(100);
  }

  float raw_span = max_val - baseline;
  if (raw_span < 0) raw_span = 0;
  alcohol_promile = (raw_span / 700.0) * 4.0;
  alcohol_promile = constrain(alcohol_promile, 0.0, 4.0);

  displayResult();
}

// ── Výsledok – len číslo ────────────────────────────────────────────────────
void displayResult() {
  display.clearDisplay();

  // Malý label hore
  display.setTextSize(1);
  display.setCursor(44, 0);
  display.print("o/oo");

  // Veľké číslo vycentrované
  display.setTextSize(4);
  char buf[8];
  dtostrf(alcohol_promile, 4, 2, buf);
  // Centrovanie: 4 znaky × ~24px = 96px → x = (128-96)/2 = 16
  display.setCursor(16, 18);
  display.print(buf);

  // Buzzer podľa hodnoty
  if (alcohol_promile >= 0.50) {
    for (int i = 0; i < 4; i++) { tone(BUZZER_PIN, 800, 300); delay(400); }
  } else if (alcohol_promile >= 0.20) {
    tone(BUZZER_PIN, 1200, 400);
  } else {
    tone(BUZZER_PIN, 1800, 300);
  }

  display.display();
  delay(6000);
}

// ── Live display ─────────────────────────────────────────────────────────────
void updateDisplay() {
  int raw = analogRead(MQ3_PIN);
  float raw_span = raw - baseline;
  if (raw_span < 0) raw_span = 0;
  float live_promile = (raw_span / 700.0) * 4.0;
  live_promile = constrain(live_promile, 0.0, 4.0);

  int bat_raw = analogRead(BATTERY_PIN);
  float bat_v = (bat_raw / 1023.0) * 5.0 * (float)(10.0 + 1.0) / 1.0;
  int bat_pct = (int)(((bat_v - 3.5) / (4.2 - 3.5)) * 100.0);
  bat_pct = constrain(bat_pct, 0, 100);

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("DREGER");
  drawBattery(98, 0, bat_pct);
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setTextSize(3);
  char buf[8];
  dtostrf(live_promile, 4, 2, buf);
  display.setCursor(8, 18);
  display.print(buf);

  display.setTextSize(1);
  display.setCursor(10, 54);
  display.print("[START] na meranie");

  display.display();
}


void drawBattery(int x, int y, int pct) {
  display.drawRect(x, y + 1, 24, 8, SSD1306_WHITE);
  display.fillRect(x + 24, y + 2, 3, 4, SSD1306_WHITE);
  int fill = (int)(20.0 * pct / 100.0);
  if (fill > 0) display.fillRect(x + 2, y + 3, fill, 4, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(x - 28, y + 1);
  display.print(pct);
  display.print("%");
}

void bootScreen() {
  display.clearDisplay();
  display.display();
  tone(BUZZER_PIN, 1800, 150);
  delay(150);
  tone(BUZZER_PIN, 1800, 150);
  delay(1500);
}
