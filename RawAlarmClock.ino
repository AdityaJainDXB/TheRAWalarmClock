/*
  The Raw Alarm Clock — Firmware
  Board: Seeed XIAO-ESP32-C3
  Display: ST7789, 284x76 (unusual portrait panel, per BLARE guide)

  PIN MAPPING (from your actual schematic, screenshot "Screenshot_2026-09-20_at_12_13_47_PM.png"):
    D4  -> DC    (display data/command)
    D5  -> CS    (display chip select)
    D6  -> BL    (display backlight)
    D8  -> RST   (display reset)
    D9  -> SCL   (display SPI clock)
    D10 -> SDA   (display SPI data/MOSI)
    D7  -> Buzzer (BZ1)
    D0  -> Button 1 (SW1) -> MODE  (cycle: normal / set hour / set minute / set alarm hour / set alarm minute)
    D1  -> Button 2 (SW2) -> UP    (increase value in set mode)
    D2  -> Button 3 (SW3) -> DOWN  (decrease value in set mode)
    D3  -> Button 4 (SW4) -> ALARM TOGGLE / SNOOZE-DISMISS

  NOTE: your schematic wasn't fully legible on exactly which SW (1-4) lands on which
  D-pin (0-3) — the mapping above is my best read. If a button does the wrong thing when
  you test it, just swap which BTN_* constant below points to which pin; the logic
  itself doesn't care which physical button is which.

  Libraries needed (Library Manager):
    - Adafruit GFX
    - Adafruit ST7789 (this pulls in ST7735 dependency too)
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// ---------- Pin definitions ----------
#define TFT_DC   4
#define TFT_CS   5
#define TFT_BL   6
#define TFT_RST  8
#define TFT_SCLK 9
#define TFT_MOSI 10

#define BUZZER_PIN 7

#define BTN_MODE  0   // SW1
#define BTN_UP    1   // SW2
#define BTN_DOWN  2   // SW3
#define BTN_ALARM 3   // SW4

// ---------- Display offset fix (same trick as the BLARE guide) ----------
class MyST7789 : public Adafruit_ST7789 {
public:
  MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst)
    : Adafruit_ST7789(cs, dc, mosi, sclk, rst) {}
  void setOffsets(uint8_t col, uint8_t row) {
    _colstart = _colstart2 = col;
    _rowstart = _rowstart2 = row;
  }
};

MyST7789 tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// ---------- Clock state ----------
unsigned long lastTickMillis = 0;
int hours = 0, minutes = 0, seconds = 0;
int alarmHour = 7, alarmMinute = 0;
bool alarmEnabled = false;
bool alarmRinging = false;

// Modes: 0 = normal display, 1 = set hour, 2 = set minute, 3 = set alarm hour, 4 = set alarm minute
int mode = 0;
const int MODE_COUNT = 5;

// ---------- Simple debounced button reader ----------
struct Button {
  uint8_t pin;
  bool lastState;
  unsigned long lastChangeMs;
};

Button btnMode  = { BTN_MODE,  HIGH, 0 };
Button btnUp    = { BTN_UP,    HIGH, 0 };
Button btnDown  = { BTN_DOWN,  HIGH, 0 };
Button btnAlarm = { BTN_ALARM, HIGH, 0 };

const unsigned long DEBOUNCE_MS = 40;

// Returns true exactly once when the button transitions from released -> pressed
bool wasPressed(Button &b) {
  bool current = digitalRead(b.pin); // buttons wired active-LOW with internal pullup
  bool pressedEdge = false;
  if (current != b.lastState) {
    if (millis() - b.lastChangeMs > DEBOUNCE_MS) {
      b.lastChangeMs = millis();
      if (current == LOW) { // pressed
        pressedEdge = true;
      }
      b.lastState = current;
    }
  }
  return pressedEdge;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_ALARM, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, LOW); // this panel's backlight is active LOW per the BLARE example

  tft.init(76, 284);          // panel size (portrait)
  tft.setOffsets(82, 18);     // offsets for the unusual resolution
  tft.invertDisplay(false);
  tft.setRotation(1);         // landscape; use 3 if it's upside down
  tft.fillScreen(ST77XX_BLACK);
  Serial.println("TFT Initialized!");

  lastTickMillis = millis();
}

// ---------- Time keeping ----------
void tickClock() {
  if (millis() - lastTickMillis >= 1000) {
    lastTickMillis += 1000;
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) hours = 0;
      }
    }
    checkAlarm();
  }
}

void checkAlarm() {
  if (alarmEnabled && !alarmRinging && hours == alarmHour && minutes == alarmMinute && seconds == 0) {
    alarmRinging = true;
  }
}

// ---------- Buzzer ----------
unsigned long lastBeepToggle = 0;
bool beepState = false;

void handleBuzzer() {
  if (alarmRinging) {
    if (millis() - lastBeepToggle > 400) {
      lastBeepToggle = millis();
      beepState = !beepState;
      digitalWrite(BUZZER_PIN, beepState ? HIGH : LOW);
    }
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// ---------- Button handling ----------
void handleButtons() {
  // MODE button: cycle modes, or dismiss ringing alarm if it's currently going off
  if (wasPressed(btnMode)) {
    if (alarmRinging) {
      alarmRinging = false;
    } else {
      mode = (mode + 1) % MODE_COUNT;
    }
  }

  // ALARM button: toggle alarm on/off (also dismisses ringing alarm)
  if (wasPressed(btnAlarm)) {
    if (alarmRinging) {
      alarmRinging = false;
    } else {
      alarmEnabled = !alarmEnabled;
    }
  }

  // UP / DOWN only do something while in a "set" mode
  if (mode == 1) { // set hour
    if (wasPressed(btnUp))   { hours = (hours + 1) % 24; }
    if (wasPressed(btnDown)) { hours = (hours + 23) % 24; }
  } else if (mode == 2) { // set minute
    if (wasPressed(btnUp))   { minutes = (minutes + 1) % 60; }
    if (wasPressed(btnDown)) { minutes = (minutes + 59) % 60; }
  } else if (mode == 3) { // set alarm hour
    if (wasPressed(btnUp))   { alarmHour = (alarmHour + 1) % 24; }
    if (wasPressed(btnDown)) { alarmHour = (alarmHour + 23) % 24; }
  } else if (mode == 4) { // set alarm minute
    if (wasPressed(btnUp))   { alarmMinute = (alarmMinute + 1) % 60; }
    if (wasPressed(btnDown)) { alarmMinute = (alarmMinute + 59) % 60; }
  }
}

// ---------- Display ----------
void drawTwoDigits(int value, int x, int y, uint16_t color) {
  char buf[3];
  snprintf(buf, sizeof(buf), "%02d", value);
  tft.setCursor(x, y);
  tft.setTextColor(color, ST77XX_BLACK);
  tft.print(buf);
}

void updateDisplay() {
  static int lastDrawnMode = -1;
  static int lastH = -1, lastM = -1, lastS = -1;
  static bool lastAlarmEnabled = false;

  if (mode != lastDrawnMode) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.setTextColor(ST77XX_WHITE);
    switch (mode) {
      case 0: tft.print("CLOCK");        break;
      case 1: tft.print("SET HOUR");     break;
      case 2: tft.print("SET MINUTE");   break;
      case 3: tft.print("SET ALM HOUR"); break;
      case 4: tft.print("SET ALM MIN");  break;
    }
    lastDrawnMode = mode;
    lastH = lastM = lastS = -1; // force redraw of time digits
  }

  tft.setTextSize(4);
  uint16_t hColor = (mode == 1) ? ST77XX_YELLOW : ST77XX_WHITE;
  uint16_t mColor = (mode == 2) ? ST77XX_YELLOW : ST77XX_WHITE;

  if (mode <= 2) {
    if (hours != lastH)   { drawTwoDigits(hours, 20, 30, hColor); lastH = hours; }
    tft.setCursor(85, 30); tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); tft.print(":");
    if (minutes != lastM) { drawTwoDigits(minutes, 110, 30, mColor); lastM = minutes; }
  } else {
    uint16_t ahColor = (mode == 3) ? ST77XX_YELLOW : ST77XX_CYAN;
    uint16_t amColor = (mode == 4) ? ST77XX_YELLOW : ST77XX_CYAN;
    if (alarmHour != lastH)   { drawTwoDigits(alarmHour, 20, 30, ahColor); lastH = alarmHour; }
    tft.setCursor(85, 30); tft.setTextColor(ST77XX_CYAN, ST77XX_BLACK); tft.print(":");
    if (alarmMinute != lastM) { drawTwoDigits(alarmMinute, 110, 30, amColor); lastM = alarmMinute; }
  }

  // Alarm status indicator, bottom-left
  if (alarmEnabled != lastAlarmEnabled || lastDrawnMode == mode) {
    tft.setTextSize(1);
    tft.setCursor(0, 60);
    tft.setTextColor(alarmEnabled ? ST77XX_GREEN : ST77XX_RED, ST77XX_BLACK);
    tft.print(alarmEnabled ? "ALARM ON " : "ALARM OFF");
    lastAlarmEnabled = alarmEnabled;
  }

  if (alarmRinging) {
    tft.setTextSize(1);
    tft.setCursor(150, 60);
    tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
    tft.print("RINGING! press any button");
  }
}

// ---------- Main loop ----------
void loop() {
  tickClock();
  handleButtons();
  handleBuzzer();
  updateDisplay();
  delay(20); // small delay keeps button polling responsive without hammering the CPU
}
