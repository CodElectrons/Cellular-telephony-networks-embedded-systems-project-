#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Keypad.h>

// ─── LCD ────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ─── GSM ────────────────────────────────────────
SoftwareSerial gsm(2, 3);  // RX, TX

// ─── RELAY ──────────────────────────────────────
#define RELAY1 12
#define RELAY2 13

// ─── KEYPAD ─────────────────────────────────────
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {11, 10, 9, 8};
byte colPins[COLS] = {7, 6, 5, 4};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ─── STATES ─────────────────────────────────────
bool relay1State = false;
bool relay2State = false;
bool inMenu = false;
int menuLevel = 0;         // 0 = home, 1 = main menu, 2 = sub-menu
int menuIndex = 0;         

String lastSMSCommand = "No command yet";
String lastSMSSender  = "";
unsigned long lastSMSTime = 0;

String incomingCaller = "";
bool incomingCallActive = false;

// ─── Menu Items ─────────────────────────────────
const char* mainMenu[] = {
  "1. Relays Control",
  "2. Make Call",
  "3. Send SMS",
  "4. Last SMS Cmd",
  "*: Back"
};
const int mainMenuCount = 5;

const char* relayMenu[] = {
  "R1 Toggle",
  "R2 Toggle",
  "Both ON",
  "Both OFF",
  "*: Back"
};
const int relayMenuCount = 5;

// ─── TIMERS ─────────────────────────────────────
unsigned long lastUpdate = 0;
const unsigned long STATUS_UPDATE_INTERVAL = 1500; 

// ─── PROTOTYPES ─────────────────────────────────
void showHome();
void showMainMenu();
void showRelayMenu();
void showLastSMS();
void handleMenuInput(char key);
void toggleRelay1();
void toggleRelay2();
void processGSM();
void updateDisplay();

// ─── SETUP ──────────────────────────────────────
void setup() {
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);

  lcd.init();
  lcd.backlight();

  Serial.begin(115200);
  gsm.begin(9600);
  delay(1500);

  // GSM init
  gsm.println("AT");
  delay(800);
  gsm.println("AT+CLIP=1");     // Caller ID
  delay(800);
  gsm.println("AT+CMGF=1");     // Text mode
  delay(800);
  gsm.println("AT+CNMI=2,2,0,0,0");  // New SMS direct show

  showHome();
}

// ─── LOOP ───────────────────────────────────────
void loop() {
  processGSM();

  char key = keypad.getKey();
  if (key) {
    if (!inMenu) {
      // Home screen shortcuts
      if (key == 'A') { inMenu = true; menuLevel = 1; menuIndex = 0; showMainMenu(); }
      if (key == '1') { toggleRelay1(); showHome(); }
      if (key == '2') { toggleRelay2(); showHome(); }
      if (key == 'B') { /* Quick call – add phone number */ }
      if (key == 'C') { /* Quick SMS – add phone number */ }
    } else {
      handleMenuInput(key);
    }
  }

  if (millis() - lastUpdate > STATUS_UPDATE_INTERVAL) {
    updateDisplay();
    lastUpdate = millis();
  }
}

// ─── Screen Display ────────────────────────
void showHome() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("R1:");
  lcd.print(relay1State ? "ON " : "OFF");
  lcd.print(" R2:");
  lcd.print(relay2State ? "ON" : "OFF");

  lcd.setCursor(0,1);
  if (incomingCallActive) {
    lcd.print("Call from ");
    lcd.print(incomingCaller.substring(0,8));
  } else {
    lcd.print("A:Menu  B:Call");
  }
}

// ─── Main Menu ──────────────────────────────────
void showMainMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Main Menu");
  lcd.setCursor(0,1);
  lcd.print(mainMenu[menuIndex]);
}

// ─── Relay Sub Menu ─────────────────────────────
void showRelayMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Relay Control");
  lcd.setCursor(0,1);
  lcd.print(relayMenu[menuIndex]);
}

// ─── Last SMS Info ──────────────────────────────
void showLastSMS() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Last: ");
  lcd.print(lastSMSCommand.substring(0,10));
  lcd.setCursor(0,1);
  lcd.print(lastSMSSender.substring(0,12));
}

// ─── Handling Menu with keypad inputs ─────
void handleMenuInput(char key) {
  if (menuLevel == 1) {  // Main menu
    if (key == 'A' || key == '#') {  // Select
      if (menuIndex == 0) { menuLevel = 2; menuIndex = 0; showRelayMenu(); }
      if (menuIndex == 3) { showLastSMS(); delay(4000); showMainMenu(); }
    }
    if (key == '*' || key == 'D') { inMenu = false; showHome(); }
    if (key == '2' || key == '8') {
      menuIndex = (menuIndex + 1) % mainMenuCount;
      showMainMenu();
    }
  }
  else if (menuLevel == 2) {  // Relay menu
    if (key == 'A' || key == '#') {
      if (menuIndex == 0) toggleRelay1();
      if (menuIndex == 1) toggleRelay2();
      if (menuIndex == 2) { digitalWrite(RELAY1, HIGH); digitalWrite(RELAY2, HIGH); relay1State = relay2State = true; }
      if (menuIndex == 3) { digitalWrite(RELAY1, LOW);  digitalWrite(RELAY2, LOW);  relay1State = relay2State = false; }
      showRelayMenu();
      delay(800);
    }
    if (key == '*' || key == 'D') { menuLevel = 1; showMainMenu(); }
    if (key == '2' || key == '8') {
      menuIndex = (menuIndex + 1) % relayMenuCount;
      showRelayMenu();
    }
  }
}

// ─── GSM Processing  ────────────────
void processGSM() {
  static String buffer = "";
  while (gsm.available()) {
    char c = gsm.read();
    buffer += c;
    if (c == '\n') {
      buffer.trim();
      String up = buffer;
      up.toUpperCase();

      if (buffer.indexOf("RING") >= 0) {
        incomingCallActive = true;
      }

      if (buffer.indexOf("+CLIP:") >= 0) {
        int s = buffer.indexOf('"') + 1;
        int e = buffer.indexOf('"', s);
        incomingCaller = buffer.substring(s, e);
        showHome();
      }

      // NO CARRIER / BUSY / Hang up
      if (buffer.indexOf("NO CARRIER") >= 0 || buffer.indexOf("BUSY") >= 0) {
        incomingCallActive = false;
        incomingCaller = "";
        showHome();
      }

      // SMS → +CMT: "+201xx","","2026/03/04,23:45:12+08"
      if (buffer.startsWith("+CMT:")) {
        delay(100);
        if (gsm.available()) {
          String msg = gsm.readStringUntil('\n');
          msg.trim();
          String umsg = msg;
          umsg.toUpperCase();

          lastSMSCommand = msg;
          lastSMSSender = incomingCaller;
          lastSMSTime = millis();

          if (umsg.indexOf("R1 ON") >= 0)  { digitalWrite(RELAY1, HIGH); relay1State = true; }
          if (umsg.indexOf("R1 OFF") >= 0) { digitalWrite(RELAY1, LOW);  relay1State = false; }
          if (umsg.indexOf("R2 ON") >= 0)  { digitalWrite(RELAY2, HIGH); relay2State = true; }
          if (umsg.indexOf("R2 OFF") >= 0) { digitalWrite(RELAY2, LOW);  relay2State = false; }

          showHome();
        }
      }

      buffer = "";
    }
  }
}

// ─── Helper Functions  ────────────────────────────────
void toggleRelay1() {
  relay1State = !relay1State;
  digitalWrite(RELAY1, relay1State);
}

void toggleRelay2() {
  relay2State = !relay2State;
  digitalWrite(RELAY2, relay2State);
}

void updateDisplay() {
  if (!inMenu) {
    showHome();
  }
}