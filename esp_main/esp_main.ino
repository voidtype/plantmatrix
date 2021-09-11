#include <TimeLib.h>
#include <time.h>

/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include <ArduinoJson.h>

#include "config.h";

//for handling  construction of time from string
struct tm tm;
time_t t;

ESP8266WiFiMulti WiFiMulti;

const String DEFAULT_TIME = "2020-01-01T00:00:01.000000+10:00";

String _time = DEFAULT_TIME;
String bpm = "0.25";
String duty = "0.004"; 
String ledState = "1";
String pressureMax = "72";

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

 // WiFi.mode(WIFI_STA);
 WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  while ((WiFi.status() != WL_CONNECTED)) {
    Serial.print(".");
    delay(200);

  }
 _setTime();
}

void loop() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    
    setConfig();
    if( _time == DEFAULT_TIME){
      _setTime();
    }

  }
  sendConfig();
  delay(10000);
}

void setConfig(){
  
          String payload = webGetter("https://c7aa99eb-dcff-46d5-bde3-303cad81eeef.mock.pstmn.io/gow");
         // Serial.println(payload);
          StaticJsonDocument<512> doc;
         DeserializationError err = deserializeJson(doc, payload);
        if (err == DeserializationError::Ok) 
          {
            // Print the values
            // (we must use as<T>() to resolve the ambiguity)
            //Serial.print("datetime = ");
            duty = doc["duty"].as<String>();
            bpm = doc["bpm"].as<String>();
            ledState = doc["ledState"].as<String>();
            pressureMax = doc["pressureMax"].as<String>();
            
          } 
          else 
          {
            // Print error to the "debug" serial port
            //Serial.print("deserializeJson() returned ");
            //Serial.println(err.c_str());
        
            // Flush all bytes in the "link" serial port buffer

        }
        //Serial.println(duty+" "+bpm);
}

String webGetter(String URL){
    WiFiClientSecure client;
    client.setInsecure(); 
    HTTPClient http;

    //Serial.print("[HTTP] begin...\n");
    if (http.begin(client,URL)) {  // HTTP


      //Serial.print("[HTTP] GET...\n");
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          return http.getString();
         } else {
          return "fff";
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
      return "fff";
    }
}
}

//renamed as there is a conflict with the contained time library function
void _setTime(){

          String payload = webGetter( "https://worldtimeapi.org/api/timezone/Australia/Sydney");
         // Serial.println(payload);
          StaticJsonDocument<512> doc;
         DeserializationError err = deserializeJson(doc, payload);
        if (err == DeserializationError::Ok) 
          {
            // Print the values
            // (we must use as<T>() to resolve the ambiguity)
            //Serial.print("datetime = ");
            _time = doc["unixtime"].as<String>();
          } 
          else 
          {
            // Print error to the "debug" serial port
            //Serial.print("deserializeJson() returned ");
            //Serial.println(err.c_str());
        
            // Flush all bytes in the "link" serial port buffer

        }
        //strptime("2020-01-01T00:00:01.000000+10:00", "%Y-%m-%dT%H:%M:%S", &tm);
        //setTime(mktime(&tm));
        setTime(_time.toInt());
        //Serial.println(now());
        

}

void sendConfig(){
  StaticJsonDocument<200> doc;
  doc["b"] = bpm;
  doc["t"] = String(now());
  doc["d"] = duty;
  doc["p"] = pressureMax;
  doc["l"] = ledState;
  serializeJson(doc, Serial);
}
