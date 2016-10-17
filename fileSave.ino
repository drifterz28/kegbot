#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

ESP8266WiFiMulti WiFiMulti;

ESP8266WebServer server = ESP8266WebServer(80);

#include <ArduinoJson.h>
#include "FS.h"

const char* defaultAPssid     = "KegBot";
const char* defaultAPpassword = "kegbotknowsyou";

const char* ssid     = "";
const char* password = "";

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  defaultAPssid = json["apSsid"];
  defaultAPpassword = json["apPassword"];

  // Real world application would store these values in some variables for
  // later use.


  return true;
}

bool saveConfig() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["apSsid"] = defaultAPssid;
  json["apPassword"] = defaultAPpassword;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
}

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected to kegbot</h1>");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Mounting FS...");

  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  if (!saveConfig()) {
    Serial.println("Failed to save config");
  } else {
    Serial.println("Config saved");
  }

  if (!loadConfig()) {
    Serial.println("Failed to load config");
  } else {
    Serial.println("Config loaded");
    Serial.println("Configuring access point...");
    Serial.print("Loaded AP SSID: ");
    Serial.println(defaultAPssid);
    Serial.print("Loaded AP Password: ");
    Serial.println(defaultAPpassword);
    WiFi.softAP(defaultAPssid, defaultAPpassword);
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.on("/", handleRoot);
    server.begin();
    Serial.println("HTTP server started");
  }
}

void loop() {

}
