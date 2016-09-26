// Use the AVRISP mkII programer

const char* ssid     = "";
const char* password = "";

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include "HX711.h"

#include "DHT.h"

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// set up scales to pins
float scaleReset = -646.38;
// scale 1
const byte hx711_data_pin1 = 5;
const byte hx711_clock_pin1 = 4;
HX711 hx711_1(hx711_data_pin1, hx711_clock_pin1);

// scale 2
const byte hx711_data_pin2 = 0;
const byte hx711_clock_pin2 = 2;
HX711 hx711_2(hx711_data_pin2, hx711_clock_pin2);

// dht22 temp sensor
#define DHTPIN 14
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// setting up non blocking timing for temp update
unsigned long previousMillis = 0;
const long interval = 1000;

float getHumidity() {
  float humidity = dht.readHumidity();
  if (isnan(humidity)) {
    Serial.println("bad humidity, re-run");
    delay(1000);
    humidity = getHumidity();
  }
  Serial.print("Humidity: ");
  Serial.println(humidity);
  return humidity;
}

float getTemp() {
  // Read temperature as Fahrenheit
  float temp_f = dht.readTemperature(true);
  if (isnan(temp_f)) {
    Serial.println("bad temp, rerun");
    delay(1000);
    temp_f = getTemp();
  }
  Serial.print("Temperature: ");
  Serial.println(temp_f);
  return temp_f;
}

String jsonOut() {
  float temp = getTemp();
  int kegOne = hx711_1.get_units(10);
  int kegTwo = hx711_2.get_units(10);
  return "{'temp': " + String(temp) + ", 'kegOne': " + String(kegOne) + ", 'kegTwo': " + String(kegTwo) + "}";
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
    webSocket.broadcastTXT("broadcastTXT");
  }
}

void handleRoot() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", jsonOut());
}

void setup() {
  Serial.begin(115200);

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP(ssid, password);

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  // keeps from sending ssid for connection
  WiFi.softAP("esp8266-15e", "nothingyoucanfindout", 1, 1);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // handle index
  server.on("/", handleRoot);
  server.on("/home", []() {
    // send index.html
    server.send(200, "text/html", "<html><body><ul id=\"data\"></ul><script>var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);connection.onopen = function() {connection.send('Connect ' + new Date());};connection.onerror = function(error) {console.log('WebSocket Error ', error);};connection.onmessage = function(e) {console.log('Server: ', e.data);showData(e.data);};function showData(data) {data = JSON.parse(data);var dataElm = document.getElementById('data');console.log(dataElm);var list = '';for (var variable in data) {if (data.hasOwnProperty(variable)) {list += '<li>' + variable + ': ' + data[variable];}}dataElm.innerHTML = list;}</script></body></html>");
  });

  server.begin();
  dht.begin(); // start the temp sensor
  // setup scales for oz
  hx711_1.set_scale(scaleReset);
  hx711_2.set_scale(scaleReset);
  hx711_1.tare();
  hx711_2.tare();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
}

void loop() {
  webSocket.loop();
  server.handleClient();
  pushData();
}
