#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define MQ3_PIN A0
#define BATTERY_PIN A1
#define START_BTN 2
#define POWER_BTN 5
#define BUZZER_PIN 6

float alcohol_ppm = 0;
float battery_voltage = 0;

bool sleeping = false;
bool lastStartState = HIGH;
bool lastPowerState = HIGH;

void setup() {
  pinMode(START_BTN, INPUT_PULLUP);
  pinMode(POWER_BTN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);

  bootScreen();
}

void loop() {
  handlePowerButton();

  if(!sleeping) {
    handleStartButton();

    static unsigned long last_update = 0;
    if(millis() - last_update > 300) {
      updateDisplay();
      last_update = millis();
    }
  }

  delay(10);
}

void handlePowerButton() {
  bool state = digitalRead(POWER_BTN);

  if(lastPowerState == HIGH && state == LOW) {
    sleeping = !sleeping;

    if(sleeping) {
      display.clearDisplay();
      display.display();
      tone(BUZZER_PIN, 500, 200);
    } else {
      tone(BUZZER_PIN, 1500, 200);
      bootScreen();
    }

    delay(200);
  }

  lastPowerState = state;
}

void handleStartButton() {
  bool state = digitalRead(START_BTN);

  if(lastStartState == HIGH && state == LOW) {
    tone(BUZZER_PIN, 1200, 150);
    measureAlcohol();
    delay(200);
  }

  lastStartState = state;
}

void bootScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(15,20);
  display.println("READY");
  display.display();
  delay(1000);
}

void measureAlcohol() {
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("MERANIE...");
  display.display();
  delay(2000);

  int max_val = 0;
  for(int i=0;i<10;i++){
    int val = analogRead(MQ3_PIN);
    if(val > max_val) max_val = val;
    delay(100);
  }

  alcohol_ppm = map(max_val, 100, 800, 0, 500);
  alcohol_ppm = constrain(alcohol_ppm, 0, 500);

  displayResult();
}

void updateDisplay() {
  int raw = analogRead(MQ3_PIN);
  alcohol_ppm = map(raw, 100, 800, 0, 500);
  alcohol_ppm = constrain(alcohol_ppm, 0, 500);

  int bat_raw = analogRead(BATTERY_PIN);
  battery_voltage = (bat_raw / 1023.0) * 5.0 * 11.0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("LIVE");

  display.setTextSize(2);
  display.setCursor(10,20);
  display.print(alcohol_ppm,0);
  display.print(" ppm");

  display.setCursor(0,50);
  display.setTextSize(1);
  display.print("Bat:");
  display.print(battery_voltage,1);
  display.print("V");

  display.display();
}

void displayResult() {
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(20,10);
  display.print(alcohol_ppm,0);
  display.println("ppm");

  if(alcohol_ppm > 200) tone(BUZZER_PIN, 800, 500);
  else tone(BUZZER_PIN, 1500, 200);

  display.display();
  delay(4000);
}