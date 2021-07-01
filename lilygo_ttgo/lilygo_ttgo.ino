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


// Available items
#define SWITCH1 "switch_01"
#define SWITCH2 "switch_02"
#define WATER "water_level"
#define WATER_FLOW "water_flow"
#define PUMP "pump"


// Wi-fi
const char *ssid = "Darebaci";
const char *password = "123456789Ab";
String uuid;

// water flow counter
const float calibration = 4.5;  // YF-S201
unsigned long timer = 0;
volatile byte numPulses = 0;
float flowRate = 0.0;

int sensorValue = 0;


WebServer server(80);
TFT_eSPI tft = TFT_eSPI();

const int led = 13;

bool readPin(int PIN) {
  return digitalRead(PIN);
}

String getUuid() {
  return String((int) ESP.getEfuseMac(), HEX);
}


void toggleSwitch(int PIN) {
  if (readPin(PIN)) {
    digitalWrite(PIN, LOW);
  } else {
    digitalWrite(PIN, HIGH);
  }
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


void handleStatus() {
  JSONVar response;
  response["status"] = "ok";
  response["temp"] = 15;
  response["uuid"] = getUuid();
  response[SWITCH1] = readPin(RELE_01);
  response[SWITCH2] = readPin(RELE_02);
  response[WATER] = readPin(WATER_PIN);
  response[WATER_FLOW] = getWaterFlow();
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
  } else if (request == action + SWITCH1) {
    toggleSwitch(RELE_01);
    response[SWITCH1] = readPin(RELE_01);
  } else if (request == action + SWITCH2) {
    toggleSwitch(RELE_02);
    response[SWITCH2] = readPin(RELE_02);
  } else if (request == read_prefix + WATER) {
    toggleSwitch(RELE_02);
    response[WATER] = readPin(WATER_PIN);
  } else if (request == read_prefix + WATER_FLOW) {
    response[WATER_FLOW] = getWaterFlow();
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

  pinMode(RELE_01, OUTPUT);
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

  int oldVal = sensorValue;
  sensorValue = map(analogRead(POT_PIN), 0, 4095, 13, 18);
  if (oldVal != sensorValue) {
    tft.fillRect(95, 98, 40, 20, GREEN);
    tft.setCursor(100, 105);
    tft.setTextColor(BLACK, GREEN);
    tft.println(sensorValue);
    ledcWrite(1, sensorValue); // write red component to channel 1, etc.
  }
  delay(50);


}
