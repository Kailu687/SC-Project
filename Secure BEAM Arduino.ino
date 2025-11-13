#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ===== PIN CONFIGURATION =====
const int LASER_PIN   = 9;
const int LDR_PIN     = A0;
const int LED_PIN     = 13;
const int BUZZER_PIN  = 3;

// ===== LCD =====
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== SYSTEM SETTINGS =====
const int BIT_DELAY = 300;   // 300 ms per bit → stable for LDR
int threshold = 600;         // Set by CAL
bool isReceiving = false;

// ===== LCD Helper =====
void lcdStatus(String l1, String l2) {
  lcd.clear();
  lcd.setCursor(0,0); lcd.print(l1);
  lcd.setCursor(0,1); lcd.print(l2);
}

// ===== SEND ONE BIT =====
void sendBit(bool bitVal) {
  digitalWrite(LASER_PIN, bitVal ? HIGH : LOW);
  digitalWrite(LED_PIN,   bitVal ? HIGH : LOW);

  if (bitVal) tone(BUZZER_PIN, 1200);
  else noTone(BUZZER_PIN);

  delay(BIT_DELAY);
}

// ===== SEND FULL MESSAGE =====
void transmitMessage(String msg) {
  lcdStatus("Mode: TX", "Sending...");
  Serial.println("\n[TX] Sending...");

  msg += "<END>";

  // ===== QUIET GAP BEFORE SYNC =====
  digitalWrite(LASER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
  delay(BIT_DELAY * 3);

  // ===== SEND TWO SYNC BYTES (0x55) =====
  for (int s = 0; s < 2; s++) {
    byte sync = 0x55;  // 01010101
    for (int b = 7; b >= 0; b--) {
      sendBit((sync >> b) & 1);
    }
  }

  // ===== SEND PAYLOAD =====
  for (int i = 0; i < msg.length(); i++) {
    char c = msg[i];
    for (int b = 7; b >= 0; b--) {
      sendBit((c >> b) & 1);
    }
  }

  // Turn off
  digitalWrite(LASER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  lcdStatus("TX Complete", "");
  Serial.println("[TX] Done.\n");
}

// ===== AUTO CALIBRATION =====
void autoCalibrate() {
  lcdStatus("CAL", "Cover LDR...");
  Serial.println("\n[CAL] Starting...");

  long darkSum = 0, brightSum = 0;

  delay(1500);
  for (int i=0; i<50; i++) { darkSum += analogRead(LDR_PIN); delay(20); }
  int darkAvg = darkSum / 50;

  Serial.print("[CAL] Dark avg = ");
  Serial.println(darkAvg);

  lcdStatus("CAL", "Shine Laser...");
  delay(2000);

  for (int i=0; i<50; i++) { brightSum += analogRead(LDR_PIN); delay(20); }
  int brightAvg = brightSum / 50;

  Serial.print("[CAL] Bright avg = ");
  Serial.println(brightAvg);

  threshold = (darkAvg + brightAvg) / 2;

  lcdStatus("CAL Done", "Thresh=" + String(threshold));
  Serial.print("[CAL] Threshold = ");
  Serial.println(threshold);
  Serial.println("[CAL] Calibration complete.\n");
}

// ===== READ ONE BIT =====
bool readBit() {
  int val = analogRead(LDR_PIN);
  bool bitVal = (val < threshold);

  digitalWrite(LED_PIN, bitVal ? HIGH : LOW);
  noTone(BUZZER_PIN);   // avoid noise during RX

  return bitVal;
}

// ===== RECEIVING LOOP =====
void receiveLoop() {
  lcdStatus("Mode: RX", "Listening...");
  Serial.println("[RX] Listening...");

  delay(1000);  // Allow TX to begin

  String msg = "";
  char c = 0;
  int bitCount = 0;

  while (isReceiving) {

    bool bitVal = readBit();
    delay(BIT_DELAY);

    c = (c << 1) | bitVal;
    bitCount++;

    if (bitCount == 8) {
      msg += (char)c;
      Serial.print((char)c);

      if (msg.endsWith("<END>")) {
        msg.remove(msg.length() - 5);

        lcdStatus("RX:", msg.substring(0,16));

        Serial.println("\n\n[RX] MESSAGE RECEIVED:");
        Serial.println(msg);
        Serial.println("------------------------");

        isReceiving = false;   // ✅ stop receiving immediately
        break;
      }

      c = 0;
      bitCount = 0;
    }

    if (Serial.available()) {
      Serial.read();
      isReceiving = false;
      lcdStatus("RX Exit", "");
      Serial.println("\n[RX] Exiting...");
      break;
    }
  }

  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);
}

// ===== SETUP =====
void setup() {
  pinMode(LASER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LASER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  lcd.init();
  lcd.backlight();
  lcdStatus("SeCure BEAM", "Ready");

  Serial.begin(9600);
  Serial.println("===== LASER LINK READY =====");
  Serial.println("Commands:");
  Serial.println("  TX <msg>");
  Serial.println("  RX");
  Serial.println("  CAL");
  Serial.println("=================================\n");
}

// ===== MAIN LOOP =====
void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("TX ")) {
      transmitMessage(input.substring(3));
    }

    else if (input.equalsIgnoreCase("RX")) {
      isReceiving = true;
      receiveLoop();
    }

    else if (input.equalsIgnoreCase("CAL")) {
      autoCalibrate();
    }

    else {
      Serial.println("Use: TX <msg> / RX / CAL");
    }
  }
}
