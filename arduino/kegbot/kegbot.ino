
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>

#include <Q2HX711.h>
#include <math.h>
#include <ArduinoJson.h>
#include "FS.h";

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

const byte hx711_data_pin = D2;
const byte hx711_clock_pin = D3;

Q2HX711 hx711(hx711_data_pin, hx711_clock_pin);

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

void setup() {
  Serial.begin(115200);

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFiMulti.addAP("SSID", "passpasspass");

  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // handle index
  server.on("/", []() {
    // send index.html
    server.send(200, "text/html", "<html><head><script>var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);connection.onopen = function () {  connection.send('Connect ' + new Date()); }; connection.onerror = function (error) {    console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);};function sendRGB() {  var r = parseInt(document.getElementById('r').value).toString(16);  var g = parseInt(document.getElementById('g').value).toString(16);  var b = parseInt(document.getElementById('b').value).toString(16);  if(r.length < 2) { r = '0' + r; }   if(g.length < 2) { g = '0' + g; }   if(b.length < 2) { b = '0' + b; }   var rgb = '#'+r+g+b;    console.log('RGB: ' + rgb); connection.send(rgb); }</script></head><body>LED Control:<br/><br/>R: <input id=\"r\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" onchange=\"sendRGB();\" /><br/>G: <input id=\"g\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" onchange=\"sendRGB();\" /><br/>B: <input id=\"b\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" onchange=\"sendRGB();\" /><br/></body></html>");
  });

  server.begin();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
}

double thermistor(int RawADC) {
  double Temp;
  Serial.println(RawADC);
  Temp = log(10000.0 * ((1024.0 / RawADC - 1)));
  Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp )) * Temp );
  Temp = Temp - 273.15;
  Temp = (Temp * 9.0) / 5.0 + 32.0;
  return Temp;
}

String jsonOut(int temp, int kegOne, int kegTwo) {
  StaticJsonBuffer<200> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["temp"] = temp;
  root["kegOne"] = kegOne;
  root["kegTwo"] = kegTwo;
  root.printTo(Serial);

  return "{temp: " + String(temp) + ", kegOne: " + String(kegOne) + ", kegTwo: " + String(kegTwo) + "}";
}

void otherThing() {
  int val = analogRead(D1);
  int kegOneVal = analogRead(D2);
  int kegTwoVal = analogRead(D3);
  double temp = thermistor(val);
  Serial.print(jsonOut(temp, kegOneVal, kegTwoVal));
  Serial.println(hx711.read() / 100.0);
  delay(500);
}

void loop() {
  webSocket.loop();
  server.handleClient();
}

