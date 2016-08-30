#include <Q2HX711.h>
#include <math.h>
#include <ArduinoJson.h>
#include "FS.h";

const byte hx711_data_pin = D2;
const byte hx711_clock_pin = D3;

Q2HX711 hx711(hx711_data_pin, hx711_clock_pin);

void setup() {
  Serial.begin(115200);
  delay(10);
  
}

double thermistor(int RawADC) {
 double Temp;
 Serial.println(RawADC);
 Temp = log(10000.0 * ((1024.0 / RawADC - 1))); 
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp )) * Temp );
 Temp = Temp - 273.15;          
 Temp = (Temp * 9.0)/ 5.0 + 32.0; 
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

void loop() {  
  int val = analogRead(D1);
  int kegOneVal = analogRead(D2);
  int kegTwoVal = analogRead(D3);
  double temp = thermistor(val);
  Serial.print(jsonOut(temp, kegOneVal, kegTwoVal));
  Serial.println(hx711.read() / 100.0);
  delay(500);
}

