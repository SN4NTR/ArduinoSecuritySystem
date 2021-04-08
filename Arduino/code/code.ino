#include <SoftwareSerial.h>
#include "DHT.h"
#include "LiquidCrystal.h"

#define RED_LED_PIN 2
#define GREEN_LED_PIN 3
#define DHT_PIN 4
#define DHT_TYPE DHT11
#define GAS_SENSOR_PIN 9
#define FLAME_SENSOR_PIN 10
#define SOUNDER_PIN 11
#define D7 53
#define D6 51
#define D5 49
#define D4 47
#define EN 45
#define RS 43

#define RX_PIN 13
#define TX_PIN 8

String telephoneNumber = "12345678";
float sensorValue;
boolean isSmsSent;

SoftwareSerial gsmSerial(RX_PIN, TX_PIN);
DHT dht(DHT_PIN, DHT_TYPE);
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

void setup() {
  Serial.begin(9600);

  pinMode(FLAME_SENSOR_PIN, INPUT);
  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(DHT_PIN, INPUT);

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(SOUNDER_PIN, OUTPUT);

  gsmSerial.begin(9600);
  gsmSerial.println("AT"); // handshake

  dht.begin();
  lcd.begin(16, 2);
}

void loop() {
  if (isDangerousSituation()) {
    processDangerousSituation();
  } else if (isWet()) {
    processWetSituation();
  } else {
    processOkSituation();
  }

  delay(200);
}

boolean isDangerousSituation() {
  int flameValue = digitalRead(FLAME_SENSOR_PIN);
  int gasValue = digitalRead(GAS_SENSOR_PIN);
  Serial.print("Flame Value: ");
  Serial.println(flameValue);
  lcd.setCursor(0, 0);
  lcd.print("Flame: ");
  lcd.print(flameValue);
  
  Serial.print("Gas Value: ");
  Serial.println(gasValue);
  lcd.setCursor(0, 1);
  lcd.print("Gas: ");
  lcd.print(gasValue);

  delay(100);

  lcd.clear();

  return gasValue == HIGH || flameValue == HIGH;
}

boolean isWet() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  Serial.print("Humidity: ");
  Serial.println(humidity);
  lcd.setCursor(0, 0);
  lcd.print("Hum: ");
  lcd.print(humidity);
  
  Serial.print("Temperature: ");
  Serial.println(temperature);
  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  lcd.print(temperature);

  delay(100);

  lcd.clear();

  return humidity >= 70;
}

void processWetSituation() {
  Serial.println("Wet!");
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(SOUNDER_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);
}

void processDangerousSituation() {
  Serial.println("WARNING!");
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(SOUNDER_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, LOW);
  sendWarningMessage();
}

void processOkSituation() {
  Serial.println("OK");
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(SOUNDER_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  isSmsSent = false;
}

void sendWarningMessage() {
  if (!isSmsSent) {
    Serial.println("Sending SMS to " + telephoneNumber);

    gsmSerial.println("AT+CMGF=1"); // Setup text mode
    gsmSerial.println("AT+CMGS=\"" + telephoneNumber + "\"\r");
    gsmSerial.println("Warning! Propability of fire!");
    gsmSerial.println((char) 26); // Ctrl + Z symbol to mark that it is SMS

    isSmsSent = true;
  }
}
