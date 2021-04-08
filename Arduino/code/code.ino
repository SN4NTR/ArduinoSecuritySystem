#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define RED_LED_PIN 2
#define GREEN_LED_PIN 3

#define DHT_PIN 4
#define DHT_TYPE DHT11

#define GAS_SENSOR_PIN 9
#define FLAME_SENSOR_PIN 10
#define SOUNDER_PIN 11

#define I2C_ADRESS 0x27
#define LCD_ROWS_AMOUNT 4
#define LCD_COLUMNS_AMOUNT 20

#define RX_PIN 13
#define TX_PIN 8

String telephoneNumber = "12345678";
boolean isSmsSent;

int flameValue;
int gasValue;
int temperature;
int humidity;

SoftwareSerial gsmSerial(RX_PIN, TX_PIN);
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal_I2C lcd(I2C_ADRESS, LCD_COLUMNS_AMOUNT, LCD_ROWS_AMOUNT);

void setup() {
  Serial.begin(9600);

  initPins();
  initGsm();
  initDhtSensor();
  initLcd();
}

void initPins() {
  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
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

void loop() {
  readSensors();
  displayInformation();

  if (isDangerousSituation()) {
    processDangerousSituation();
  } else if (isWet()) {
    processWetSituation();
  } else {
    processOkSituation();
  }

  delay(100);
}

void readSensors() {
  gasValue = digitalRead(GAS_SENSOR_PIN);
  flameValue = digitalRead(FLAME_SENSOR_PIN);
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void displayInformation() {
  lcd.clear();

  printOnLcd(0, 0, "Flame: " + String(flameValue));
  printOnLcd(1, 0, "Gas: " + String(gasValue));
  printOnLcd(2, 0, "Humidity: " + String(humidity));
  printOnLcd(3, 0, "Temperature: " + String(temperature));
}

boolean isDangerousSituation() {
  return gasValue == HIGH || flameValue == HIGH;
}

boolean isWet() {
  return humidity >= 70;
}

void printOnLcd(int row, int column, String text) {
  lcd.setCursor(column, row);
  lcd.print(text);
}

void processWetSituation() {
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(SOUNDER_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void processDangerousSituation() {
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(SOUNDER_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);

  lcd.clear();
  printOnLcd(0, 0, "WARNING!");
  printOnLcd(1, 0, "Propability of fire!");
  sendWarningMessage();
}

void processOkSituation() {
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(SOUNDER_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  isSmsSent = false;
}

void sendWarningMessage() {
  if (!isSmsSent) {
    gsmSerial.println("AT+CMGF=1"); // Setup text mode
    gsmSerial.println("AT+CMGS=\"" + telephoneNumber + "\"\r");
    gsmSerial.println("Warning! Propability of fire!");
    gsmSerial.println((char) 26); // Ctrl + Z symbol to mark that it is SMS

    isSmsSent = true;
  }
}
