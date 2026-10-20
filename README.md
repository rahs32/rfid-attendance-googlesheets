# rfid-attendance-googlesheets

An automated, microcontroller-based RFID system designed to log attendance/access data seamlessly. This project uses an RFID reader to identify tagged cards/fobs and processes the records with real-time feedback on an LCD screen and buzzer notification.

---

## 🛠️ Components & Hardware Required

| Component | Model / Description | Quantity |
| :--- | :--- | :---: |
| Microcontroller | ESP8266 NodeMCU / ESP32 / Arduino | 1 |
| RFID Module | MFRC522 (13.56 MHz) | 1 |
| Display | 16x2 LCD Display with PCF8574 I2C Adapter | 1 |
| Audio Feedback | Active Buzzer | 1 |
| RFID Credentials | Tags / Cards (13.56 MHz) | Variable |
| Miscellaneous | Jumper wires, Breadboard / Custom PCB | 1 |

---

## 🔌 Pin Mapping & Circuit Connection

### MFRC522 RFID Module (SPI Connection)
* **VCC** $\rightarrow$ 3.3V
* **RST** $\rightarrow$ D3 (GPIO0) / Pin 9
* **GND** $\rightarrow$ GND
* **MISO** $\rightarrow$ D6 (GPIO12) / Pin 12
* **MOSI** $\rightarrow$ D7 (GPIO13) / Pin 11
* **SCK** $\rightarrow$ D5 (GPIO14) / Pin 13
* **SDA (SS)** $\rightarrow$ D4 (GPIO2) / Pin 10

### 16x2 I2C LCD Display
* **GND** $\rightarrow$ GND
* **VCC** $\rightarrow$ 5V
* **SDA** $\rightarrow$ D2 (GPIO4) / A4
* **SCL** $\rightarrow$ D1 (GPIO5) / A5

### Active Buzzer
* **Positive (+)** $\rightarrow$ D8 (GPIO15)
* **Negative (-)** $\rightarrow$ GND

------------------------------------------------------------------------------------------------------------------------------------------------
