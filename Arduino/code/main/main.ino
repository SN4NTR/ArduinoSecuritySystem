#include <SoftwareSerial.h>
#include <Wire.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define RED_LED_PIN 2
#define GREEN_LED_PIN 3
#define GREEN_YELLOW_PIN 4

#define DHT_PIN 5
#define DHT_TYPE DHT11

#define MQ135_SENSOR_PIN 9
#define MQ7_SENSOR_PIN 10
#define MQ2_SENSOR_PIN 11
#define FLAME_SENSOR_PIN 12
#define SOUNDER_PIN 13

#define LCD_I2C_ADRESS 0x20
#define KEYPAD_I2C_ADDRESS 0x21

#define RX_PIN 22
#define TX_PIN 8

const byte ROWS = 4;
const byte COLUMNS = 3;

const byte LCD_ROWS_AMOUNT = 4;
const byte LCD_COLUMNS_AMOUNT = 20;

const byte HUMIDITY_MAX_BORDER = 70;
const byte TEMPERATURE_MAX_BORDER = 80;

const char KEYBOARD_KEYS[ROWS][COLUMNS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
const byte ROW_PINS[ROWS] = {0, 1, 2, 3};
const byte COLUMN_PINS[COLUMNS] = {4, 5, 6};

enum SystemStatus {
  CO,
  CO_2,
  SMOKE,
  FLAME,
  TEMPERATURE,
  HUMIDITY,
  OK
};

const SoftwareSerial gsmSerial(RX_PIN, TX_PIN);
const DHT dht(DHT_PIN, DHT_TYPE);
const LiquidCrystal_I2C lcd(LCD_I2C_ADRESS, LCD_COLUMNS_AMOUNT, LCD_ROWS_AMOUNT);
const Keypad_I2C keypad(makeKeymap(KEYBOARD_KEYS), ROW_PINS, COLUMN_PINS, ROWS, COLUMNS, KEYPAD_I2C_ADDRESS);

boolean isDeviceConfigured;
boolean isSmsSent;
byte flameValue;
byte smokeValue;
byte coValue;
byte co2Value;
byte temperature;
byte humidity;
String mobileNumber = "+";
SystemStatus systemStatus = OK;

void setup() {
  Serial.begin(9600);

  initPins();
  initGsm();
  initDhtSensor();
  initLcd();
  initKeypad();
}

void initPins() {
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(MQ2_SENSOR_PIN, INPUT);
  pinMode(MQ7_SENSOR_PIN, INPUT);
  pinMode(MQ135_SENSOR_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(GREEN_YELLOW_PIN, OUTPUT);
  pinMode(SOUNDER_PIN, OUTPUT);
}

void initGsm() {
  gsmSerial.begin(9600);
  gsmSerial.println("AT"); // handshake
}

void initDhtSensor() {
  dht.begin();
}

void initLcd() {
  lcd.init();
  lcd.backlight();
}

void initKeypad() {
  keypad.begin(makeKeymap(KEYBOARD_KEYS));
}

void loop() {
  if (!isDeviceConfigured) {
    configureDevice();
  } else {
    execute();
  }
}

void configureDevice() {
  printOnLcd(7, 1, "Welcome!");
  delay(1000);

  enterMobileNumber();

  isDeviceConfigured = true;
}

void enterMobileNumber() {
  lcd.clear();
  printOnLcd(2, 0, "Enter your mobile");
  printOnLcd(6, 1, "number:");
  printOnLcd(0, 2, "+");
  lcd.setCursor(1, 2);

  char key = NO_KEY;
  while (key == NO_KEY || key != '#') {
    key = keypad.getKey();
    if (key != NO_KEY && key != '#') {
      mobileNumber += key;
      lcd.print(key);
    }
  }

  lcd.clear();
  printOnLcd(4, 0, "Mobile Number");
  printOnLcd(4, 1, mobileNumber);
  printOnLcd(6, 2, "is saved");
  printOnLcd(4, 3, "successfully!");

  delay(1000);
}

void execute() {
  readSensors();

  if (isDangerousSituation()) {
    processDangerousSituation();
  } else if (isWet()) {
    processWetSituation();
  } else {
    processOkSituation();
  }

  delay(1000);
}

void readSensors() {
  smokeValue = digitalRead(MQ2_SENSOR_PIN);
  coValue = digitalRead(MQ7_SENSOR_PIN);
  co2Value = digitalRead(MQ135_SENSOR_PIN);
  flameValue = digitalRead(FLAME_SENSOR_PIN);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void displaySensorsInformation() {
  lcd.clear();
  printOnLcd(0, 0, "Flame: " + String(flameValue));
  printOnLcd(0, 1, "Smoke: " + String(smokeValue));
  printOnLcd(0, 2, "CO: " + String(coValue));
  printOnLcd(0, 3, "CO2: " + String(co2Value));

  delay(1000);

  lcd.clear();
  printOnLcd(0, 0, "Humidity: " + String(humidity) + "%");
  printOnLcd(0, 1, "Temperature: " + String(temperature) + (char) 223 + "C");
}

boolean isDangerousSituation() {
  return isSmoke() || isFlame() || isCo() || isCo2();
}

boolean isSmoke() {
  if (smokeValue == HIGH) {
    systemStatus = SMOKE;
    return true;
  }
  return false;
}

boolean isFlame() {
  if (flameValue == HIGH) {
    systemStatus = FLAME;
    return true;
  }
  return false;
}

boolean isCo() {
  if (coValue == HIGH) {
    systemStatus = CO;
    return true;
  }
  return false;
}

boolean isCo2() {
  if (co2Value == HIGH) {
    systemStatus = CO_2;
    return true;
  }
  return false;
}

boolean isWet() {
  return humidity > HUMIDITY_MAX_BORDER;
}

void printOnLcd(int column, int row, String text) {
  lcd.setCursor(column, row);
  lcd.print(text);
}

void processWetSituation() {
  digitalWrite(GREEN_YELLOW_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);

  lcd.clear();
  printOnLcd(0, 0, "Humidity value > " + String(HUMIDITY_MAX_BORDER) + "%");
  printOnLcd(5, 1, "Hood system");
  printOnLcd(5, 2, "is enabled.");
}

void processDangerousSituation() {
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(SOUNDER_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(GREEN_YELLOW_PIN, LOW);

  switch (systemStatus) {
    case CO:
      displayCoWarningMessage();
      break;
    case CO_2:
      displayCo2WarningMessage();
      break;
    case SMOKE:
      displaySmokeWarningMessage();
      break;
    case FLAME:
      displayFlameWarningMessage();
      break;
  }

  sendWarningMessage();
}

void displayCoWarningMessage() {
  lcd.clear();
  printOnLcd(7, 0, "WARNING!");
  printOnLcd(3, 1, "CO is detected!");
  printOnLcd(2, 2, "SMS notification");
  printOnLcd(7, 3, "is sent.");
}

void displayCo2WarningMessage() {
  lcd.clear();
  printOnLcd(7, 0, "WARNING!");
  printOnLcd(2, 1, "CO2 is detected!");
  printOnLcd(2, 2, "SMS notification");
  printOnLcd(7, 3, "is sent.");
}

void displaySmokeWarningMessage() {
  lcd.clear();
  printOnLcd(7, 0, "WARNING!");
  printOnLcd(1, 1, "Smoke is detected!");
  printOnLcd(2, 2, "SMS notification");
  printOnLcd(7, 3, "is sent.");
}

void displayFlameWarningMessage() {
  lcd.clear();
  printOnLcd(7, 0, "WARNING!");
  printOnLcd(1, 1, "Flame is detected!");
  printOnLcd(2, 2, "SMS notification");
  printOnLcd(7, 3, "is sent.");
}

void processOkSituation() {
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(SOUNDER_PIN, LOW);
  digitalWrite(GREEN_YELLOW_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  displaySensorsInformation();

  systemStatus = OK;
  isSmsSent = false;
}

void sendWarningMessage() {
  if (!isSmsSent) {
    switch (systemStatus) {
      case CO:
        sendCoWarningMessage();
        break;
      case CO_2:
        sendCo2WarningMessage();
        break;
      case SMOKE:
        sendSmokeWarningMessage();
        break;
      case FLAME:
        sendFlameWarningMessage();
        break;
    }

    isSmsSent = true;
  }
}

void sendCoWarningMessage() {
  gsmSerial.println("AT+CMGF=1"); // Setup text mode
  gsmSerial.println("AT+CMGS=\"" + mobileNumber + "\"\r");
  gsmSerial.println("Warning! CO is detected!");
  gsmSerial.println((char) 26); // Ctrl + Z symbol to mark that it is SMS
}

void sendCo2WarningMessage() {
  gsmSerial.println("AT+CMGF=1"); // Setup text mode
  gsmSerial.println("AT+CMGS=\"" + mobileNumber + "\"\r");
  gsmSerial.println("Warning! CO2 is detected!");
  gsmSerial.println((char) 26); // Ctrl + Z symbol to mark that it is SMS
}

void sendFlameWarningMessage() {
  gsmSerial.println("AT+CMGF=1"); // Setup text mode
  gsmSerial.println("AT+CMGS=\"" + mobileNumber + "\"\r");
  gsmSerial.println("Warning! Flame is detected!");
  gsmSerial.println((char) 26); // Ctrl + Z symbol to mark that it is SMS
}

void sendSmokeWarningMessage() {
  gsmSerial.println("AT+CMGF=1"); // Setup text mode
  gsmSerial.println("AT+CMGS=\"" + mobileNumber + "\"\r");
  gsmSerial.println("Warning! Smoke is detected!");
  gsmSerial.println((char) 26); // Ctrl + Z symbol to mark that it is SMS
}
