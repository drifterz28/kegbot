// ***Use the AVRISP mkII programer***
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <ArduinoJson.h>
#include "FS.h"

#include "HX711.h"
#include "DHT.h"

const char* ssid = "";
const char* password = "";

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// dht22 temp sensor
#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// setting up non blocking timing for temp update
unsigned long previousMillis = 0;
const long interval = 1000;

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

String jsonOut() {
  float temp = getTemp();
  float humidity = getHumidity();
  float kegOne = hx711_1.get_units(10);
  float kegTwo = hx711_2.get_units(10);
  return "{\"temp\": " + String(temp) + ", \"humidity\": " + String(humidity) + ", \"kegOne\": " + String(kegOne) + ", \"kegTwo\": " + String(kegTwo) + "}";
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
        webSocket.sendTXT(num, "Connected");
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

void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  Serial.println(jsonOut());
  server.send(200, "application/json", jsonOut());
}

void handleSetup() {
  Serial.println("setup");
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>KegBot Setup</title></head><body><form method=\"post\" action=\"/save\" style=\"margin: 0 auto;max-width:500px;width:100%;\"><h3>Wifi setup</h3><table><tr><td><label>SSID:</label></td><td><input name=\"ssid\"/></td></tr><tr><td><label>Password:</label></td><td><input name=\"password\"/></td></tr></table><button>Save</button></form></body></html>");
}

void handleHome() {
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>KegBot</title><meta name=\"viewport\" content=\"user-scalable=no, initial-scale=1, maximum-scale=1, minimum-scale=1, width=device-width\"><link href=\"https://fonts.googleapis.com/css?family=Roboto\" rel=\"stylesheet\"><link rel=\"stylesheet\" href=\"assets/main.css\"/><script src=\"https://unpkg.com/react@15.3.1/dist/react.js\"></script><script src=\"https://unpkg.com/react-dom@15.3.1/dist/react-dom.js\"></script><script src=\"https://unpkg.com/babel-core@5.8.38/browser.min.js\"></script></head><body><div class=\"container\"></div><script type=\"text/babel\" src=\"assets/main.js\"></script></body></html>");
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
}

void loop() {
  webSocket.loop();
  server.handleClient();
  pushData();
}
