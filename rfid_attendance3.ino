/* ------------------------------------------------------------------------
 * Smart RFID Attendance System (ESP8266 + RC522 + I2C LCD + Google Sheets)
 * 
 * Fixes Applied:
 *  1. Corrected payload key to "sheet_name": "Attendance"
 *  2. Formatted payload to map directly to your Google Sheet columns:
 *     Col A: Date & Time (Google Script generated)
 *     Col B: UID
 *     Col C: Student (First Name + Last Name)
 *     Col D: Roll No (Student ID from Block 4)
 *     Col E: Status ("Present")
 *     Col F: Parent Phone (Phone from Block 8)
 *  3. Fixed null-terminator ('\0') in read buffer to prevent JSON corruption
 *  4. Set BUZZER to D8 (GPIO 15) to prevent hardware conflict with LCD SDA (D2)
 *  5. Added proper card halt and crypto reset after each tap
 * ------------------------------------------------------------------------ */

#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "SPI.h"
#include "MFRC522.h"
#include "HTTPSRedirect.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"

// I2C 16x2 LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Google Apps Script Deployment ID
const char *GScriptId = "AKfycbw5BgE7uJYU8qTADSkAgsnEGXAImcqtgCzng2-d5kYsPh__7yg0luzSN8ArlU4via4syA";

// Network credentials
const char* ssid     = "Rahul";
const char* password = "priya83@";

// JSON Base Payload (sheet_name matches your "Attendance" tab)
String payload_base = "{\"command\": \"insert_row\", \"sheet_name\": \"Attendance\", \"values\": ";
String payload = "";

// Google Sheets Connection Parameters
const char* host       = "script.google.com";
const int   httpsPort  = 443;
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect* client  = nullptr;

// Pin Definitions
#define RST_PIN    D3  // GPIO 0
#define SS_PIN     D4  // GPIO 2
#define BUZZER_PIN D8  // GPIO 15 (Avoid D2, which is I2C SDA for LCD)

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;

// 18-byte read buffer for 16-byte MIFARE blocks
byte readBlockData[18];

void beep(int ms) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(ms);
  digitalWrite(BUZZER_PIN, LOW);
}

void ReadDataFromBlock(int blockNum, byte buffer[]) { 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Auth failed for block "));
    Serial.println(blockNum);
    return;
  }

  byte bufferLen = 18;
  status = mfrc522.MIFARE_Read(blockNum, buffer, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Reading failed for block "));
    Serial.println(blockNum);
    return;
  }

  // Null-terminate string at 16 bytes to prevent JSON corruption
  buffer[16] = '\0';
}

void setup() {
  Serial.begin(9600);
  delay(10);
  Serial.println('\n');

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  SPI.begin();
  mfrc522.PCD_Init();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("WiFi...");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to");
  lcd.setCursor(0, 1);
  lcd.print("Google Cloud...");

  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  bool connected = false;
  for (int i = 0; i < 5; i++) { 
    if (client->connect(host, httpsPort) == 1) {
      connected = true;
      break;
    }
    Serial.println("Retrying Google connection...");
    delay(1000);
  }

  if (connected) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("System Ready");
    lcd.setCursor(0, 1);
    lcd.print("Scan ID Card");
    beep(100);
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cloud Conn Fail");
  }
}

void loop() {
  if (client != nullptr && !client->connected()) {
    client->connect(host, httpsPort);
  }

  // Check for card presence
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  beep(80); // Card detected beep

  // 1. Extract Hardware UID
  String uidStr = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidStr += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uidStr += String(mfrc522.uid.uidByte[i], HEX);
  }
  uidStr.toUpperCase();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Reading Card...");

  // 2. Read stored blocks from RFID
  ReadDataFromBlock(4, readBlockData);
  String rollNo = String((char*)readBlockData);
  rollNo.trim();

  ReadDataFromBlock(5, readBlockData);
  String firstName = String((char*)readBlockData);
  firstName.trim();

  ReadDataFromBlock(6, readBlockData);
  String lastName = String((char*)readBlockData);
  lastName.trim();

  ReadDataFromBlock(8, readBlockData);
  String parentPhone = String((char*)readBlockData);
  parentPhone.trim();

  String studentName = firstName + " " + lastName;
  studentName.trim();
  if (studentName.length() == 0) studentName = "Student";

  // Display on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(studentName.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print("Logging Data...");

  // 3. Build CSV string aligned with Sheet columns: UID, Student, Roll No, Status, Parent Phone
  String values = "\"" + uidStr + "," + studentName + "," + rollNo + ",Present," + parentPhone + "\"}";
  payload = payload_base + values;

  Serial.println("\nPublishing payload:");
  Serial.println(payload);

  // 4. Send POST request to Google Apps Script
  if (client->POST(url, host, payload)) {
    lcd.setCursor(0, 1);
    lcd.print("Status: Present ");
    beep(150);
  } else {
    Serial.println("POST request failed.");
    lcd.setCursor(0, 1);
    lcd.print("Sync Failed!    ");
  }

  // 5. Release card session
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  delay(3500); // Debounce to allow removing card

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print("Scan ID Card");
}