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

#include <StreamUtils.h>


#include "config.h";

//for handling  construction of time from string
struct tm tm;
time_t t;

ESP8266WiFiMulti WiFiMulti;

const String DEFAULT_TIME = "2020-01-01T00:00:01.000000+10:00";

String _time = DEFAULT_TIME;
String bpm = "0.25";
String duty = "0.004"; 
String ledSetting = "255";
String pressureMax = "72";

String psi;
String solenoidState;
String pumpState;
String ledState;

const int REFRESH_PERIOD = 10000;
unsigned long time_now;

bool stateUpdated = false;
bool configUpdated = false;

void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  //Serial.println();
  //Serial.println();
  //Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    //Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  

 // WiFi.mode(WIFI_STA);
 WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  while ((WiFi.status() != WL_CONNECTED)) {
    //Serial.print(".");
    delay(200);

  }
 _setTime();
   time_now = millis();

}

void postState(){
  String payload = webGetter("http://144.202.8.17:81/readings/?device=48a6118c-f22e-48b9-b4c6-3bcdd6d8641d&ledState="+ledState+"&pumpState="+pumpState+"&solenoidState="+solenoidState+"&psi="+psi);
  //invalidate the state Update if we've sent it
  if (payload != "fff"){
    stateUpdated = false;
  }
  
}

void _main(){
  // don't bother executing yet if we've lost wifi
  if ((WiFi.status() == WL_CONNECTED)) {
    //get config from django server
    setConfig();
    //the input buffer should be clear,so it should be safe to send config to arduino
    if(configUpdated)
    {
      sendConfig();
    }
    if( _time == DEFAULT_TIME){
      _setTime();
    }
    //if state has been updated, post it to the django server
    if (stateUpdated){
      postState();
    }
    if (Serial.available()){
      //get the state from esp
      getState();

    }
  }
}

void loop() {
  //nb REFRESH_PERIOD = 10000
  if(millis() > time_now + REFRESH_PERIOD){
      time_now = millis();
      _main();
  }

}

void setConfig(){
  
          String payload = webGetter("http://144.202.8.17:81/configs/48a6118c-f22e-48b9-b4c6-3bcdd6d8641d/");
         // Serial.println(payload);
          StaticJsonDocument<512> doc;
         DeserializationError err = deserializeJson(doc, payload);
        if (err == DeserializationError::Ok) 
          {
            String _duty = doc["duty"].as<String>();
            String _bpm = doc["bpm"].as<String>();
            String _ledSetting = doc["ledState"].as<String>();
            String _pressureMax = doc["pressureMax"].as<String>();

            if(!configUpdated){
            configUpdated = (duty != _duty || 
                              bpm != _bpm ||
                              ledSetting != _ledSetting ||
                              pressureMax != _pressureMax);
            }

            if (configUpdated){
              duty = _duty;
              bpm = _bpm;
              ledSetting = _ledSetting;
              pressureMax = _pressureMax;
            }
            
          } 
          else 
          {
            // Print error to the "debug" serial port
            //Serial.print("deserializeJson() returned ");
            //Serial.println(err.c_str());
        
            // Flush all bytes in the "link" serial port buffer

        }
}

String webGetter(String URL){
    WiFiClient client;
//    client.setInsecure(); 
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
        //Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      //Serial.printf("[HTTP} Unable to connect\n");
      return "fff";
    }
}
}

//renamed as there is a conflict with the contained time library function
void _setTime(){

          String payload = webGetter( "http://worldtimeapi.org/api/timezone/Australia/Sydney");
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
        

}

void sendConfig(){
  StaticJsonDocument<200> doc;
  doc["a"] = "u";
  doc["b"] = bpm;
  //doc["t"] = String(now());
  doc["d"] = duty;
  doc["p"] = pressureMax;
  doc["l"] = ledSetting;
  serializeJson(doc, Serial);
  Serial.flush();
}

void getState(){
   if (Serial.available()) 
  {
    // Allocate the JSON document
    StaticJsonDocument<500> doc;
    Serial.setTimeout(100);
    // Read the JSON document from the "link" serial port
    ReadLoggingStream loggingClient(Serial, Serial);
    DeserializationError err = deserializeJson(doc, loggingClient);

    if (err == DeserializationError::Ok) 
    {
     if (doc["a"] == "e")
      {
        // Print the values
        // (we must use as<T>() to resolve the ambiguity)
        psi = doc["psi"].as<String>();
        solenoidState = doc["s"].as<String>();
        pumpState = doc["p"].as<String>();
        ledState = doc["l"].as<String>();
        stateUpdated = true;
      } 
      else if (doc["a"]=="ack")
      {
        configUpdated = false;
      }
    }
    {
  
      // If there's garbag, flush all bytes in the serial  buffer
      while (Serial.available() > 0)
        Serial.read();
    }
  }
}
