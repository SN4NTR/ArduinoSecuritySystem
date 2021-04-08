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

#define GAS_SENSOR_PIN 9
#define FLAME_SENSOR_PIN 10
#define SOUNDER_PIN 11

#define LCD_I2C_ADRESS 0x20
#define KEYPAD_I2C_ADDRESS 0x21

#define RX_PIN 13
#define TX_PIN 8

const byte ROWS = 4;
const byte COLUMNS = 3;

const byte LCD_ROWS_AMOUNT = 4;
const byte LCD_COLUMNS_AMOUNT = 20;

boolean isDeviceConfigured;
boolean isSmsSent;
int flameValue;
int gasValue;
int temperature;
int humidity;

int humidityMaxBorder = 70;
String mobileNumber = "+";

char keys[ROWS][COLUMNS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {0, 1, 2, 3};
byte columnPins[COLUMNS] = {4, 5, 6};

SoftwareSerial gsmSerial(RX_PIN, TX_PIN);
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(LCD_I2C_ADRESS, LCD_COLUMNS_AMOUNT, LCD_ROWS_AMOUNT);
Keypad_I2C keypad(makeKeymap(keys), rowPins, columnPins, ROWS, COLUMNS, KEYPAD_I2C_ADDRESS);

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
  pinMode(GAS_SENSOR_PIN, INPUT);
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
  keypad.begin(makeKeymap(keys));
}

void loop() {
  if (!isDeviceConfigured) {
    configureDevice();
  } else {
    execute();
  }
}

void configureDevice() {
  printOnLcd(0, 1, "      Welcome!");
  delay(1000);

  enterMobileNumber();

  isDeviceConfigured = true;
}

void enterMobileNumber() {
  lcd.clear();
  printOnLcd(0, 0, " Enter your mobile");
  printOnLcd(0, 1, "     number:");
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
  printOnLcd(0, 0, "   Mobile Number");
  printOnLcd(3, 1, mobileNumber);
  printOnLcd(0, 2, "     is saved");
  printOnLcd(0, 3, "   successfully!");

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
  gasValue = digitalRead(GAS_SENSOR_PIN);
  flameValue = digitalRead(FLAME_SENSOR_PIN);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void displaySensorsInformation() {
  lcd.clear();

  printOnLcd(0, 0, "Flame: " + String(flameValue));
  printOnLcd(0, 1, "Gas: " + String(gasValue));
  printOnLcd(0, 2, "Humidity: " + String(humidity) + "%");
  printOnLcd(0, 3, "Temperature: " + String(temperature) + (char) 223 + "C");
}

boolean isDangerousSituation() {
  return gasValue == HIGH || flameValue == HIGH;
}

boolean isWet() {
  return humidity > humidityMaxBorder;
}

void printOnLcd(int column, int row, String text) {
  lcd.setCursor(column, row);
  lcd.print(text);
}

void processWetSituation() {
  digitalWrite(GREEN_YELLOW_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);

  lcd.clear();
  printOnLcd(0, 0, "Humidity value > " + String(humidityMaxBorder) + "%");
  printOnLcd(0, 1, "    Hood system");
  printOnLcd(0, 2, "    is enabled.");
}

void processDangerousSituation() {
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(SOUNDER_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(GREEN_YELLOW_PIN, LOW);

  lcd.clear();
  printOnLcd(0, 0, "       WARNING!");
  printOnLcd(0, 1, "Propability of fire!");
  printOnLcd(0, 2, "  SMS notification");
  printOnLcd(0, 3, "      is sent.");
  sendWarningMessage();
}

void processOkSituation() {
  displaySensorsInformation();

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(SOUNDER_PIN, LOW);
  digitalWrite(GREEN_YELLOW_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  isSmsSent = false;
}

void sendWarningMessage() {
  if (!isSmsSent) {
    gsmSerial.println("AT+CMGF=1"); // Setup text mode
    gsmSerial.println("AT+CMGS=\"" + mobileNumber + "\"\r");
    gsmSerial.println("Warning! Propability of fire!");
    gsmSerial.println((char) 26); // Ctrl + Z symbol to mark that it is SMS

    isSmsSent = true;
  }
}
