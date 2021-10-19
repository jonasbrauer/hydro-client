#include <Arduino_JSON.h>

// display/fonts
#include "NotoSansBold15.h"
#define AA_FONT_SMALL NotoSansBold15
#define GREEN 0x52B788
#define BLACK 0x0000

// Hudro-HUB URL
#define HUB "http://www.hydroserver.home/rest/devices/register"

#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPClient.h>


// Available physical pins
#define RELE_01 17
#define RELE_02 2
#define WATER_PIN 27
#define WATER_FLOW_PIN 15
#define PUMP_PIN 33
#define POT_PIN 32
#define ULTRA_TRIG 32
#define ULTRA_ECHO 25


// available items
// controls
#define SWITCH1 "switch_01"
#define SWITCH2 "switch_02"
#define PUMP_SPEED "pump_speed"
// sensors
#define WATER "water_level"
#define WATER_FLOW "water_flow"
#define TANK_PERCENTAGE "tank_percentage"


// Wi-fi
const char *ssid = "Darebaci";
const char *password = "123456789Ab";
String uuid;

// water flow counter
const float calibration = 4.5;  // YF-S201
unsigned long timer = 0;
volatile byte numPulses = 0;
float flowRate = 0.0;

// pump speed setting (analog out for the MOSFET)
float oldPumpSpeed = 0; // set on change only
float pumpSpeed = 0;


WebServer server(80);
TFT_eSPI tft = TFT_eSPI();


// ================ READS ================ //


bool readPin(int PIN) {
  return digitalRead(PIN);
}

String getUuid() {
  return String((int) ESP.getEfuseMac(), HEX);
}

float getWaterFlow() {
  if ((millis() - timer) > 1000) {
    // vypnutí detekce přerušení po dobu výpočtu a tisku výsledku
    detachInterrupt(WATER_FLOW_PIN);
    // výpočet průtoku podle počtu pulzů za daný čas
    // se započtením kalibrační konstanty
    flowRate = ((1000.0 / (millis() - timer)) * numPulses) / calibration;

    // nulování počítadla pulzů
    numPulses = 0;
    timer = millis();
    attachInterrupt(WATER_FLOW_PIN, addPulse, FALLING);
  }
  return flowRate;
}

float getTankPercentage() {
  float totalHeight = 52; // cm
  digitalWrite(ULTRA_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRA_TRIG, HIGH);
  delayMicroseconds(5);
  digitalWrite(ULTRA_TRIG, LOW);
  // pulse length [us]
  float odezva = pulseIn(ULTRA_ECHO, HIGH);
  // cm
  float actualHeight = odezva / 58.31;
  return (1 - (actualHeight / totalHeight)) * 100;
}

// ================ WRITES ================ //

void toggleSwitch(int PIN) {
  if (readPin(PIN)) {
    digitalWrite(PIN, LOW);
  } else {
    digitalWrite(PIN, HIGH);
  }
}

void setPumpSpeed(float newSpeed) {
  pumpSpeed = newSpeed;
}


// ================ SERVER HANDLES ================ //

void handleStatus() {
  JSONVar response;
  response["status"] = "ok";
  response["uuid"] = getUuid();

  // sensors
  JSONVar temp;
  temp["value"] = 15;
  temp["type"] = "sensor";
  temp["unit"] = "°C";
  response["temp"] = JSON.stringify(temp);

  JSONVar waterLevel;
  waterLevel["value"] = readPin(WATER_PIN) ? "false" : "true";
  waterLevel["type"] = "sensor";
  temp["unit"] = "bool";
  response[WATER] = JSON.stringify(waterLevel);

  JSONVar waterLevelPerc;
  waterLevelPerc["value"] = getTankPercentage();
  waterLevelPerc["type"] = "sensor";
  temp["unit"] = "float";
  response[TANK_PERCENTAGE] = JSON.stringify(waterLevelPerc);

  JSONVar waterFlow;
  waterFlow["value"] = getWaterFlow();
  waterFlow["type"] = "sensor";
  waterFlow["unit"] = "l/min";
  response[WATER_FLOW] = JSON.stringify(waterFlow);

  // controls
  JSONVar sw1;
  sw1["value"] = readPin(RELE_01) ? "false" : "true";
  sw1["type"] = "control";
  sw1["input"] = "bool";
  response[SWITCH1] = JSON.stringify(sw1);

  JSONVar sw2;
  sw2["value"] = readPin(RELE_02) ? "false" : "true";
  sw2["type"] = "control";
  sw2["input"] = "bool";
  response[SWITCH2] = JSON.stringify(sw2);

  JSONVar pump;
  pump["value"] = pumpSpeed;
  pump["type"] = "control";
  pump["input"] = "float";
  response[PUMP_SPEED] = JSON.stringify(pump);
  
  
  server.send(200, "application/json", JSON.stringify(response));
}

void handleAction() {
  String body = server.arg("plain");
  JSONVar myObject = JSON.parse(body);

  if (JSON.typeof(myObject) == "undefined") {
    handleError("Parsing JSON failed.");
    return;
  }
  if (!myObject.hasOwnProperty("request")) {
    handleError("JSON needs to have exactly one keyword <request>");
    return;
  }

  String read_prefix = "read_";
  String action = "action_";
  String request = (const char*) myObject["request"];
  if (request == "status") {
     handleStatus();
     return;
  }

  JSONVar response;
  if (request == "info") {
    response["uuid"]= getUuid();
    
  // ACTION
  } else if (request == action + SWITCH1) {
    toggleSwitch(RELE_01);
    response["value"] = readPin(RELE_01) ? "false" : "true";
    
  } else if (request == action + SWITCH2) {
    toggleSwitch(RELE_02);
    response["value"] = readPin(RELE_02) ? "false" : "true";
    
  } else if (request == action + PUMP_SPEED) {
    if (!myObject.hasOwnProperty("value")) {
      handleError("Value property needed.");
      return;
    }
    float newSpeed = JSON.stringify(myObject["value"]).toFloat();
    if (newSpeed < 0 || newSpeed > 1) {
      handleError("Value represents %, it needs to be between 0-1.");
      return;
    }
    setPumpSpeed(newSpeed);
    response["value"] = newSpeed;

  // READ
  } else if (request == read_prefix + SWITCH1) {
    response["value"] = readPin(RELE_01) ? "false" : "true";

  } else if (request == read_prefix + SWITCH2) {
    response["value"] = readPin(RELE_02) ? "false" : "true";
  
  } else if (request == read_prefix + WATER) {
    response["value"] = readPin(WATER_PIN) ? "false" : "true";
    
  } else if (request == read_prefix + WATER_FLOW) {
    response["value"] = getWaterFlow();
    
  } else {
    handleError("UNKNOWN_CMD " + request);
    return;
  }
  response["status"] = "ok";
  server.send(200, "application/json", JSON.stringify(response));
}

void handleError(String msg) {

  JSONVar message;
  message["uri"] = server.uri();
  message["method"] = (server.method() == HTTP_GET) ? "GET" : "POST";
  message["arguments"] = server.args();
  message["cause"] = msg;
  analogRead(1);

  server.send(400, "application/json", JSON.stringify(message));
}


void printTft() {
  tft.fillScreen(BLACK);
  tft.setCursor(0, 0);
  tft.print("UUID: "); tft.setTextColor(GREEN, TFT_BLACK); tft.println(uuid);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("Connected to: "); tft.setTextColor(GREEN, TFT_BLACK); tft.println(ssid);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("IP: "); tft.setTextColor(GREEN, TFT_BLACK); tft.println(WiFi.localIP());
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
}


void setup(void) {
  uuid = String((int) ESP.getEfuseMac(), HEX);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // 160x128
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.loadFont(AA_FONT_SMALL);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    tft.print(".");
  }

  // basic device info, always on display.
  printTft();

  server.on("/", HTTP_GET, handleStatus);
  server.on("/", HTTP_POST, handleAction);
  //server.onNotFound(handleNotFound);
  server.begin();

  tft.println();
  tft.println("HTTP started");

  // register this device to the hub
//  HTTPClient http;
//  http.begin(HUB);
//  String httpData = "{\"url\":\"http://";
//  httpData += WiFi.localIP().toString();
//  httpData += "\"}";
//  http.addHeader("Content-Type", "application/json");
//
//  tft.println(httpData);
//  int responseCode = http.POST(httpData);
//  tft.println(responseCode);
//  if (responseCode > 0) {
//    tft.setTextColor(GREEN, TFT_BLACK);
//    tft.println("REGISTRED");
//    tft.setTextColor(TFT_WHITE, TFT_BLACK);
//  } else {
//    tft.setTextColor(TFT_RED, TFT_BLACK);
//    tft.println("NOT REGISTRED");
//    tft.setTextColor(TFT_WHITE, TFT_BLACK);
//  }
  pinMode(ULTRA_TRIG, OUTPUT);
  pinMode(ULTRA_ECHO, INPUT);
  pinMode(RELE_01, OUTPUT);
  pinMode(RELE_02, OUTPUT);
  pinMode(WATER_PIN, INPUT);
  pinMode(WATER_FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(WATER_FLOW_PIN, addPulse, FALLING);

  ledcAttachPin(PUMP_PIN, 1);
  ledcSetup(1, 12000, 8);
  Serial.begin(9600);

  delay(1000);

  tft.setCursor(0, 105);
  tft.println("Pump speed: ");
  tft.println("Water flow: ");
  // here goes the value
  tft.setCursor(155, 120);
  tft.print("l/h");

  // #END - SETUP
}

float mapFloat(float value, float fromLow, float fromHigh, float toLow, float toHigh) {
   float x = (value - fromLow) / (fromHigh - fromLow);
   return toLow + (x * (toHigh - toLow));
}

void addPulse() {
  numPulses++;
}

void loop(void) {
  server.handleClient();

  //  printTft();
  float oldFlowRate = flowRate;
  float flow = getWaterFlow();
  if (oldFlowRate != flow) {
    int color = GREEN;
    if (flow < 1) {
      color = TFT_RED;
    }
    tft.fillRect(95, 118, 50, 20, color);
    tft.setCursor(100, 120);
    tft.setTextColor(BLACK, color);
    tft.println(getWaterFlow());
  }

  if (oldPumpSpeed != pumpSpeed) {
    tft.fillRect(95, 98, 40, 20, GREEN);
    tft.setCursor(100, 105);
    tft.setTextColor(BLACK, GREEN);
    tft.print(pumpSpeed * 100);
    tft.print(" %");
    float mappedSpeed = mapFloat(pumpSpeed, 0, 1, 14.5, 20);
    tft.print(" ("); tft.print(mappedSpeed); tft.println(")");
    ledcWrite(1, mapFloat(pumpSpeed, 0, 1, 14.5, 20));
    oldPumpSpeed = pumpSpeed;
  }
  delay(50);


}
