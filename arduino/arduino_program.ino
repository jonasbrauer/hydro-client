
#include "DHT.h"

#include "configuration.h"

#include <OneWire.h>
#include <DallasTemperature.h>

// Unique UUID for each board. Any better way to achieve this easily?
#define DEVICE_UUID "a669c4da-5eeb-11eb-ae93-0242ac130002"
//#define DEVICE_UUID "8c7448de-3d36-4e64-bf01-efa58b44d45b"

#define BUS_PIN 25
#define ADR_SIZE 8 //byte

//vytvoreni instanci
OneWire Bus(BUS_PIN);
DallasTemperature Sensors(&Bus);
DeviceAddress DevAdr;

uint8_t nSensors = 0;


#define PLOVAK 12
#define LED_G 0
#define LED_R 0
#define LED_B 0
#define RELE_01 17
#define RELE_02 13
#define RELE_03 15
#define RELE_04 2

#define pinDHT 26
#define typDHT11 DHT11     // or DHT22

#define BAUD 19200

// Available sensors/controls
#define A_TEMP "temp"
#define A_HUM "hum"
#define W_TEMP "water_temp"
#define W_LEVEL "water_level"
#define SWITCH1 "switch_01"
#define SWITCH2 "switch_02"
#define SWITCH3 "switch_03"
#define SWITCH4 "switch_04"
#define BLINK "blink"

// variables
DHT tempSensor(pinDHT, typDHT11);


void setup() {
  pinMode(pinDHT, INPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(RELE_01, OUTPUT);
  pinMode(RELE_02, OUTPUT);
  pinMode(RELE_03, OUTPUT);
  pinMode(RELE_04, OUTPUT);
  pinMode(PLOVAK, INPUT);

  pinMode(37, OUTPUT);
  pinMode(38, OUTPUT);
  pinMode(39, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(25, OUTPUT);
  pinMode(26, OUTPUT);
  pinMode(27, OUTPUT);
  pinMode(17, OUTPUT);

  Serial.begin(BAUD);
  tempSensor.begin();

  Sensors.begin();
  nSensors = Sensors.getDeviceCount();

  delay(1000);
  //vypis poctu senzoru
  Serial.print("Pocet senzoru: ");
  Serial.println(nSensors);

  Serial.print("Temp: ");
  Serial.println(Sensors.getTempCByIndex(0));
}

// the loop function runs over and over again forever
void loop() {

  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');

    String action = "action_";
    String READ = "read_";

    if (data == "info") {
      Serial.print("uuid:");
      Serial.println(getUuid());
    } else if (data == "status") {
      Serial.println(full_status());

    } else if (data == action + SWITCH1) {
      toggleSwitch(RELE_01);
      String res = "status:ok,";
      Serial.println(res + SWITCH1 + ":" + readPin(RELE_01));

    } else if (data == action + SWITCH2) {
      toggleSwitch(RELE_02);
      String res = "status:ok,";
      Serial.println(res + SWITCH2 + ":" + readPin(RELE_02));

    } else if (data == action + SWITCH3) {
      toggleSwitch(RELE_03);
      String res = "status:ok,";
      Serial.println(res + SWITCH3 + ":" + readPin(RELE_03));

    } else if (data == action + SWITCH4) {
      toggleSwitch(RELE_04);
      String res = "status:ok,";
      Serial.println(res + SWITCH4 + ":" + readPin(RELE_04));

    } else if (data == action + BLINK) {
      ok_blink(LED_G);
      Serial.println("status:ok");

    } else if (data == READ + "temp") {
      String res = "status:ok,";
      res = res + A_TEMP + ":" + read_temp();
      Serial.println(res);

    } else if (data == READ + "hum") {
      String res = "status:ok,";
      res = res + A_HUM + ":" + read_hum();
      Serial.println(res);

    } else {
      Serial.println("UNKNOWN");
    }
  }
}


String full_status() {

  String response = "uuid:" + getUuid() + ",";
  response = response + A_TEMP + ":" + read_dht_temp() + ",";
  response = response + A_HUM + ":" + read_hum() + ",";
  response = response + W_TEMP + ":" + read_temp() + ",";
  response = response + W_LEVEL + ":" + readPin(PLOVAK) + ",";
  response = response + SWITCH1 + ":" + readPin(RELE_01) + ",";
  response = response + SWITCH2 + ":" + readPin(RELE_02) + ",";
  response = response + "status:ok,";

  return response;
}


// READ OPERATIONS

String getUuid() {
  return DEVICE_UUID;
}


float read_dht_temp() {
  float temp = tempSensor.readTemperature();
  if (isnan(temp)) {
    return -1;
  }
  return temp;
}


float read_temp() {
  return Sensors.getTempCByIndex(0);
}

float read_hum() {
  float hum = tempSensor.readHumidity();
  if (isnan(hum)) {
    return -1;
  }
  return hum;
}

bool readPin(int PIN) {
  return digitalRead(PIN);
}


// WRITE OPERATIONS

void toggleSwitch(int PIN) {
  if (readPin(PIN)) {
    digitalWrite(PIN, LOW);
  } else {
    digitalWrite(PIN, HIGH);
  }
}


void ok_blink(int PIN) {
  digitalWrite(PIN, HIGH);
  delay(100);
  digitalWrite(PIN, LOW);
  delay(100);
  digitalWrite(PIN, HIGH);
  delay(100);
  digitalWrite(PIN, LOW);
}
