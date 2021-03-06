// ***Use the AVRISP mkII programer***
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>

#include "HX711.h"
#include "DHT.h"

const char* ssid = "";
const char* password = "";

String serverUrl = "http://iot-ecommsolution.rhcloud.com/kegbot";

int fanState = 0;
float temp;
float humidity;
float kegOne;
float kegTwo;

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// dht22 temp sensor
#define DHTPIN 14
#define DHTTYPE DHT22
#define FAN_PIN 10

DHT dht(DHTPIN, DHTTYPE);

// setting up non blocking timing for WS update
unsigned long previousMillis = 0;
const long interval = 1000;

// setting up non blocking timing for server update
unsigned long serverPreviousMillis = -100000;
const long serverInterval = 1000 * 60 * 1;

// set up scales to pins
float scaleReset = -646.38;

//scale 1
#define hx711_data_pin1 0
#define hx711_clock_pin1 2
HX711 hx711_1;

// scale 2
#define hx711_data_pin2 4
#define hx711_clock_pin2 5
HX711 hx711_2;

float getHumidity() {
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    Serial.println("bad humidity");
    humidity = 0;
  }
  return humidity;
}

float getTemp() {
  // Read temperature as Fahrenheit
  float temp_f = dht.readTemperature(true);
  if (isnan(temp_f)) {
    Serial.println("bad temp");
    temp_f = 0;
  }
  return temp_f;
}

void updateValues() {
  temp = getTemp();
  humidity = getHumidity();
  kegOne = hx711_1.get_units(10);
  kegTwo = hx711_2.get_units(10);
}

String jsonOut() {
  updateValues();
  return "{\"fan\":" + String(fanState) + ",\"temp\":" + String(temp) + ",\"humidity\":" + String(humidity) + ",\"kegOne\":" + String(kegOne) + ",\"kegTwo\":" + String(kegTwo) + "}";
}

void pushServerData() {
  unsigned long currentMillis = millis();
  if(currentMillis - serverPreviousMillis >= serverInterval) {
    serverPreviousMillis = currentMillis;
    String urlArgs = "?json=" + jsonOut();
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      String fullUrl = serverUrl + urlArgs;
      Serial.print("Send to server: ");
      Serial.println(fullUrl);
      HTTPClient http;
      http.begin(fullUrl);
      int httpCode = http.GET();
      if (httpCode > 0) {
        Serial.println("data sent to server");
      } else {
        Serial.println("fail");
        Serial.println(httpCode);
      }
      http.end();
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        // send message to client
        String json = jsonOut();
        webSocket.sendTXT(num, json);
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      break;
  }
}

void pushData() {
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    String json = jsonOut();
    webSocket.broadcastTXT(json);
  }
}

void handleScaleSettings() {
  // server.arg("") get args
  if(server.hasArg("reset")) {
    if(server.arg("reset") == "kegOne") {
      hx711_1.tare();
      Serial.println("kegOne tare");
    }
    if(server.arg("reset") == "kegTwo") {
      hx711_2.tare();
      Serial.println("kegTwo tare");
    }
  }
  if(server.hasArg("fan")) {
    
    if(fanState == 1) {
      fanState = 0;
    } else {
      fanState = 1;
    }
    digitalWrite(FAN_PIN, fanState);
    Serial.print("fan=");
    Serial.println(fanState);
  }
  server.send(200, "application/json", jsonOut());
}

void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  Serial.println(jsonOut());
  server.send(200, "application/json", jsonOut());
}

void handleHome() {
  String currentMillis = String(millis());
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>KegBot</title><meta name=\"viewport\" content=\"user-scalable=no, initial-scale=1, maximum-scale=1, minimum-scale=1, width=device-width\"><link href=\"https://fonts.googleapis.com/css?family=Roboto\" rel=\"stylesheet\"><link rel=\"stylesheet\" href=\"https://drifterz28.github.io/kegbot/assets/main.css?" + currentMillis + "\"/><script src=\"https://unpkg.com/react@15.3.1/dist/react.js\"></script><script src=\"https://unpkg.com/react-dom@15.3.1/dist/react-dom.js\"></script><script src=\"https://unpkg.com/babel-core@5.8.38/browser.min.js\"></script></head><body><div class=\"container\"></div><script type=\"text/babel\" src=\"https://drifterz28.github.io/kegbot/assets/main.js?" + currentMillis + "\"></script></body></html>");
}

void setup() {
  Serial.begin(115200);
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP(ssid, password);
  WiFi.softAP("esp8266", "nothingyoucanfindout", 1, 1);
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if(MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // handle routes
  server.on("/", handleRoot);
  server.on("/home", handleHome);
  server.on("/settings", handleScaleSettings);
  server.begin();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
  Serial.println("Begin scales and dht");

  hx711_1.begin(hx711_data_pin1, hx711_clock_pin1);
  hx711_2.begin(hx711_data_pin2, hx711_clock_pin2);
  dht.begin(); // start the temp sensor

  // setup scales for oz
  hx711_1.set_scale(scaleReset);
  hx711_2.set_scale(scaleReset);
  hx711_1.tare();
  hx711_2.tare();
  Serial.println("done with setup");
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
}

void loop() {
  webSocket.loop();
  server.handleClient();
  pushData();
  pushServerData();
}
