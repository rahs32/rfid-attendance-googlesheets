/* ------------------------------------------------------------------------
 * Continuous RFID Student Card Writer (ESP8266 + RC522)
 * Writes multiple cards sequentially to the same Google Sheet format.
 * ------------------------------------------------------------------------ */

#include "SPI.h"
#include "MFRC522.h"

const uint8_t RST_PIN = D3; // GPIO 0
const uint8_t SS_PIN  = D4; // GPIO 2

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

byte bufID[16];
byte bufFirstName[16];
byte bufLastName[16];
byte bufPhone[16];
byte bufAddress[16];

void getSerialString(const char* prompt, byte targetBuffer[]) {
  for (byte i = 0; i < 16; i++) targetBuffer[i] = ' ';

  Serial.println(prompt);

  char firstChar = 0;
  while (true) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c != '\r' && c != '\n') {
        firstChar = c;
        break;
      }
    }
    delay(10);
  }

  byte len = 0;
  if (firstChar != '#') {
    targetBuffer[0] = firstChar;
    Serial.setTimeout(30000);
    len = 1 + Serial.readBytesUntil('#', (char*)(targetBuffer + 1), 15);
  }

  for (byte i = len; i < 16; i++) targetBuffer[i] = ' ';

  delay(50);
  while (Serial.available() > 0) Serial.read();

  Serial.print("Buffered: ");
  for (byte i = 0; i < 16; i++) Serial.write(targetBuffer[i]);
  Serial.println("\n---------------------------------------");
}

void writeBlock(byte blockNum, byte* data, const char* label) {
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Auth failed for ");
    Serial.println(label);
    return;
  }

  status = mfrc522.MIFARE_Write(blockNum, data, 16);
  if (status == MFRC522::STATUS_OK) {
    Serial.print(label);
    Serial.println(" written.");
  } else {
    Serial.print("Write error: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  Serial.println("\n==========================================");
  Serial.println("    CONTINUOUS RFID STUDENT WRITER        ");
  Serial.println("==========================================");
}

void loop() {
  Serial.println("\n>>> PREPARING NEW STUDENT CARD <<<");
  
  // 1. Capture details for the card
  getSerialString("Enter Student Roll No / ID (end with #):", bufID);
  getSerialString("Enter First Name (end with #):", bufFirstName);
  getSerialString("Enter Last Name (end with #):", bufLastName);
  getSerialString("Enter Parent Phone Number (end with #):", bufPhone);
  getSerialString("Enter Address / Class (end with #):", bufAddress);

  // 2. Wait for card tap
  Serial.println("\nData buffered! Tap the RFID card onto the reader...");

  while (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(50);
  }

  Serial.print("Card Detected! UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // 3. Write Sector 1
  writeBlock(4, bufID, "Block 4 (Roll No)");
  writeBlock(5, bufFirstName, "Block 5 (First Name)");
  writeBlock(6, bufLastName, "Block 6 (Last Name)");

  // 4. Write Sector 2
  writeBlock(8, bufPhone, "Block 8 (Phone)");
  writeBlock(9, bufAddress, "Block 9 (Address)");

  Serial.println("\n*** CARD PROGRAMMED SUCCESSFULLY! ***");
  Serial.println("Remove the card from the reader now.");

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // 4-second delay before prompting for the next student card
  delay(4000);
}