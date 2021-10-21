#include <StreamUtils.h>
#include <stdio.h>

/**********************************
 * PLANT MATRIX main arduino code
    Copyright (C) 2021  Evan Pollack

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

///////////////////      ////////////            ////////////        ////////////    
///////////////////    /////////////////         //////////////    ///////////////// 
       ////           /////         /////        ////      ////   ////           ////
       ////           ////           ////        ////      ////   ////           ////
       ////           ////           ////        ////      ////   ////           ////
       ////           ////           ////        ////      ////   ////           ////
       ////           ////           ////        ////      ////   ////           ////
       ////           ////           ////        ////      ////   ////           ////
       ////           /////         /////        ////      ////   ////           ////
       ////            /////////////////         ////////////      ///////////////// 
       ////              ////////////            ///////////         ////////////    

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/************************************************************************************
/* 
 * Troubleshoot with high duty fast cycle (~45 sec cycle 15s on/off)
 *  
 * LED controll
 * Get time
 * get watering cycle & pressure setting from remote
 * re-implement wifi, and move the wifi config out of code, Make a basic error handler
 * Test this code thouroughly 
 */


#include <ArduinoJson.h>

const int PUMP_PIN = 2;
const int SOLENOID_PIN = 12;
const int PURGE_PIN = 13;
const int LED_PIN = 6;
const int PRESSURE_PIN = A0;
//needs to be non-const and have a calibration cycle (NB calibration failure is noncritical as long as the pump has the pressure switch)
const float offset = 0.468;

float bpm = 0.25;
float duty = 0.004;
float pressureValue = 0;
float previousPressure = 60;
unsigned long previousMillis = 0;
int solenoidState = HIGH;
int pumpState = HIGH;
int ledState = 255;
int pressureMax = 72;
bool cacheInvalid = true;
String _time = "default";

bool stateDue = true;

void getSettings()
{
   if (Serial.available()) 
  {
    // Allocate the JSON document
    // This one must be bigger than for the sender because it must store the strings
    StaticJsonDocument<500> doc;
    Serial.setTimeout(100);
    // Read the JSON document from the "link" serial port
    ReadLoggingStream loggingClient(Serial, Serial);
    DeserializationError err = deserializeJson(doc, loggingClient);

    if (err == DeserializationError::Ok && doc["a"]=="u") 
    {
      // Print the values
      // (we must use as<T>() to resolve the ambiguity)
      bpm = doc["b"].as<float>();
      //_time = doc["t"].as<String>();
      duty = doc["d"].as<float>();
      pressureMax = doc["p"].as<int>();
      ledState = doc["l"].as<int>();
      //Serial.println(_time);
      sendAck();
    } 
    // Flush all bytes in the "link" serial port buffer
    while (Serial.available() > 0)
      Serial.read();
  

  }
}

void sendAck(){
  StaticJsonDocument<128> ackDoc;
  ackDoc["a"] = "ack";
  serializeJson(ackDoc, Serial);
  Serial.flush();
}

void sendState(){
  StaticJsonDocument<128> stateDoc;
  stateDoc["a"] = "e";
  stateDoc["l"] = ledState;
  stateDoc["p"] = pumpState;
  stateDoc["s"] = solenoidState;
  stateDoc["psi"] = pressureValue;
  serializeJson(stateDoc, Serial);
  Serial.flush();
}


long onPeriod(){
  long period = (60.0*1000)/bpm;
  return (long)(period * duty);
}

long offPeriod(){
  long period = (60.0*1000)/bpm;
  return (long)(period - onPeriod());
}

void setup() {
  Serial.begin(115200);
  //sendATcmd("AT",5,"OK");
  //sendATcmd("AT+CWMODE=1",5,"OK");
  //sendATcmd("AT+CWJAP=\""+ ssid +"\",\""+ password +"\"",20,"OK");
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN,HIGH);
  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN,ledState);
  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN,HIGH);
  Serial.setTimeout(50);
  delay(1000);
  previousMillis = millis();
}

void loop() {
  unsigned long currentMillis = millis();
  //get the pressure value now as we will always need to report it
  float volts =  analogRead(PRESSURE_PIN)* 5.00 / 1024;
  //as per the spec sheet, calculate the voltage
  pressureValue = (volts-offset )* 400 * 0.145038;
  pressureValue = int(pressureValue)?pressureValue:previousPressure;
  ////url += String(pressureValue);

  //only measure the pressure based off 2 data points to avoid inconsistencies due to
  //water knock, transients not caught by the capacitor etc
  float pressureAverage = (pressureValue + previousPressure) / 2.0;
  previousPressure = pressureValue;

  // get settings cache from serial every 10 seconds, only bother trying for the first second
  //TODO : replace settings cache invalidation code
  if ((currentMillis-previousMillis)%10000 < 1000 /*&& cacheInvalid*/)
  {
    if(Serial.available())
      getSettings();
    analogWrite(LED_PIN,ledState);
    cacheInvalid = false;
    stateDue = true;
  }
  else if ((currentMillis-previousMillis)%10000 > 5000)
  {
    cacheInvalid = true;
  }

  if (stateDue && ((currentMillis-previousMillis)%10000 > 5000))
  {
   sendState();
   stateDue = false;
  }

  bool on = ((currentMillis-previousMillis) < onPeriod());
  if (on){
    if (solenoidState == LOW){
      previousMillis = currentMillis;
      ////url+= "&module3=1";
    }
    solenoidState = HIGH;
 
  }
  else{
    if((currentMillis-previousMillis) < (onPeriod() + offPeriod())){
      if (solenoidState == HIGH)
      {
        //url+= "&module3=0";
      }
       solenoidState = LOW;
    }
    else{
      previousMillis = currentMillis;
      if (solenoidState == LOW)
      {
        //url+= "&module3=1";
      }
      solenoidState = HIGH;
    }
  }

  //Serial.print(pressureValue,3);
  //Serial.println(" psi");
  if (pressureAverage < 40)
  {
    if (pumpState == LOW)
      //url+= "&module2=1";
    pumpState = HIGH; 
  }
  else if (pressureAverage < pressureMax && pumpState == HIGH)  //keep running up to 60 psi
  {
    pumpState = HIGH;
  }
  else
  {
    if (pumpState == HIGH)
    {
      //url+= "&module2=0";
    }
    pumpState = LOW;
  }
 
  //Turn on pump slightly before solenoid to allow pressure build
  digitalWrite(PUMP_PIN, pumpState);
  digitalWrite(SOLENOID_PIN, solenoidState);

}

/*void pushData(String url){

  sendATcmd("AT+CIPMUX=1", 1, "OK");
  sendATcmd("AT+CIPSTART=0, \"TCP\",\"" + host +"\"," + port, 2, "OK");
  sendATcmd("AT+CIPSEND=0," + String(url.length() + 4), 2, ">");
  
  Serial.println(url);
}

void sendATcmd(String AT_cmd, int AT_cmd_maxTime, char readReplay[]) {


  while(AT_cmd_time < (AT_cmd_maxTime)) {
    Serial.println(AT_cmd);
    if(Serial.find(readReplay)) {
      AT_cmd_result = true;
      break;
    }
  
    AT_cmd_time++;
  delay(50);
  }
  if(AT_cmd_result == true) {
    AT_cmd_time = 0;
  }
  
  if(AT_cmd_result == false) {
    AT_cmd_time = 0;
  }
  
  AT_cmd_result = false;
  delay(writeInterval);
 }*/
