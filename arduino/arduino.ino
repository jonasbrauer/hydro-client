
#include "DHT.h"

#include "configuration.h"

#include <Arduino_JSON.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Unique UUID for each board. Any better way to achieve this easily?
#define DEVICE_UUID "a669c4da-5eeb-11eb-ae93-0242ac130002"
//#define DEVICE_UUID "8c7448de-3d36-4e64-bf01-efa58b44d45b"

#define BUS_PIN 25
#define ADR_SIZE 8 //byte

// variables
OneWire Bus(BUS_PIN);
DallasTemperature Sensors(&Bus);
DeviceAddress DevAdr;
DHT tempSensor(pinDHT, typDHT11);

uint8_t nSensors = 0;


String handleError(String msg) {
  JSONVar message;
  message["status"] = "error";
  message["cause"] = msg;
  return JSON.stringify(message);
}

String handleStatus() {
  JSONVar response;
  response["status"] = "ok";
  response["temp"] = 15;
  response["uuid"] = getUuid();
  response[SWITCH1] = readPin(RELE_01);
  response[SWITCH2] = readPin(RELE_02);;
  return JSON.stringify(response);
}

String handleAction(String body) {
  JSONVar myObject = JSON.parse(body);

  if (JSON.typeof(myObject) == "undefined") {
    return handleError("Parsing JSON failed."); 
  }
  if (!myObject.hasOwnProperty("request")) {
    return handleError("JSON needs to have exactly one keyword <request>");
  }

  String read_prefix = "read_";
  String action = "action_";
  String request = (const char*) myObject["request"];
  if (request == "status") {
     return handleStatus();
  }

  JSONVar response;
  if (request == "info") {
    response["uuid"]= getUuid();
  } else if (request == action + SWITCH1) {
    toggleSwitch(RELE_01);
    response[SWITCH1] = readPin(RELE_01);
  } else if (request == action + SWITCH2) {
    toggleSwitch(RELE_02);
    response[SWITCH2] = readPin(RELE_02);
  } else {
    return handleError("UNKNOWN_CMD " + request);
  }
  response["status"] = "ok";
  return JSON.stringify(response);
}

void setup() {

  Serial.begin(BAUD);

}

// the loop function runs over and over again forever
void loop() {

  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    String response = handleAction(data);
    Serial.println(response);
  }
  delay(0.1);
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
