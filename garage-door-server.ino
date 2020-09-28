#include "config.h"

#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#else
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 14

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

uint8_t tempSensors[][8] = {
  { 0x28, 0x67, 0x4C, 0x8F, 0x1E, 0x19, 0x01, 0x6D  },
  { 0x28, 0x76, 0x92, 0x5B, 0x1E, 0x19, 0x01, 0xF2  },
  { 0x28, 0x60, 0xE6, 0x8A, 0x1E, 0x19, 0x01, 0xE7  }
};

const char* ssid = CONFIG_NETWORK_SSID;
const char* password = CONFIG_NETWORK_PASSWORD;

const char* http_username = CONFIG_LOGIN_USERNAME;
const char* http_password = CONFIG_LOGIN_PASSWORD;

AsyncWebServer server(80);

int switchPin1 = 12;//0
int switchStateCur1;
int relayPin1 = 4;//3

int switchPin2 = 13;//0
int switchStateCur2;
int relayPin2 = 5;//3

int relayBlinkPeriod = 400; // length of led blink in ms
unsigned long relayStarted[] = {0,0};

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html><html lang="en"><head><meta charset="utf-8"/><title>Garage Door Client</title><link href="http://cdn.jsdelivr.net/gh/alexrcameron/garage-door-client@4954a5701615e9eee91e4fdc7346fd644e203e54/build/static/css/main.266173d9.chunk.css" rel="stylesheet"></head><body><noscript>You need to enable JavaScript to run this app.</noscript><div id="root"></div><script>!function(e){function r(r){for(var n,l,a=r[0],i=r[1],f=r[2],p=0,s=[];p<a.length;p++)l=a[p],Object.prototype.hasOwnProperty.call(o,l)&&o[l]&&s.push(o[l][0]),o[l]=0;for(n in i)Object.prototype.hasOwnProperty.call(i,n)&&(e[n]=i[n]);for(c&&c(r);s.length;)s.shift()();return u.push.apply(u,f||[]),t()}function t(){for(var e,r=0;r<u.length;r++){for(var t=u[r],n=!0,a=1;a<t.length;a++){var i=t[a];0!==o[i]&&(n=!1)}n&&(u.splice(r--,1),e=l(l.s=t[0]))}return e}var n={},o={1:0},u=[];function l(r){if(n[r])return n[r].exports;var t=n[r]={i:r,l:!1,exports:{}};return e[r].call(t.exports,t,t.exports,l),t.l=!0,t.exports}l.m=e,l.c=n,l.d=function(e,r,t){l.o(e,r)||Object.defineProperty(e,r,{enumerable:!0,get:t})},l.r=function(e){"undefined"!=typeof Symbol&&Symbol.toStringTag&&Object.defineProperty(e,Symbol.toStringTag,{value:"Module"}),Object.defineProperty(e,"__esModule",{value:!0})},l.t=function(e,r){if(1&r&&(e=l(e)),8&r)return e;if(4&r&&"object"==typeof e&&e&&e.__esModule)return e;var t=Object.create(null);if(l.r(t),Object.defineProperty(t,"default",{enumerable:!0,value:e}),2&r&&"string"!=typeof e)for(var n in e)l.d(t,n,function(r){return e[r]}.bind(null,n));return t},l.n=function(e){var r=e&&e.__esModule?function(){return e.default}:function(){return e};return l.d(r,"a",r),r},l.o=function(e,r){return Object.prototype.hasOwnProperty.call(e,r)},l.p="/";var a=this["webpackJsonpgarage-door-client"]=this["webpackJsonpgarage-door-client"]||[],i=a.push.bind(a);a.push=r,a=a.slice();for(var f=0;f<a.length;f++)r(a[f]);var c=i;t()}([])</script><script src="http://cdn.jsdelivr.net/gh/alexrcameron/garage-door-client@4954a5701615e9eee91e4fdc7346fd644e203e54/build/static/js/2.53446011.chunk.js"></script><script src="http://cdn.jsdelivr.net/gh/alexrcameron/garage-door-client@4954a5701615e9eee91e4fdc7346fd644e203e54/build/static/js/main.47984c9c.chunk.js"></script></body></html>
)rawliteral";

String processor(const String& var){
  return String();
}

void wifi_connect() {
  // Check if we have a WiFi connection, if we don't, connect.
  int xCnt = 0;

  if (WiFi.status() != WL_CONNECTED){
    Serial.println("\n\nConnecting to " + (String)ssid);
  
    WiFi.mode(WIFI_STA);
    
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED  && xCnt < 50) {
      delay(500);
      Serial.print(".");
      xCnt ++;
    }

    if (WiFi.status() != WL_CONNECTED){
      Serial.println("Never Connected!");
    } else {
      Serial.println();
      Serial.println("WiFi connected");  
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      Serial.println(WiFi.macAddress());
    }

  } else {
    Serial.println("Wifi Already Connected");
  }
}

bool lightsAreOn() {
  bool lightsOn = true;
  int sensorValue = analogRead(A0);
  
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);
  Serial.println("Light Sensor: " + (String)voltage + "V");

  if (voltage > 4){
    lightsOn = false;
  }

  return lightsOn;
}

String getSensorData() {
  String data = "";

  data += "{ \"door1_isOpen\": ";
  data += switchStateCur1 == 1 ? "true" : "false";

  data += ", \"door2_isOpen\": ";
  data += switchStateCur2 == 1 ? "true" : "false";

  data += ", \"building_temp\": ";
  data += (String)sensors.getTempF(tempSensors[0]);

  data += ", \"fridge_temp\": ";
  data += (String)sensors.getTempF(tempSensors[1]);

  data += ", \"freezer_temp\": ";
  data += (String)sensors.getTempF(tempSensors[2]);

  data += ", \"lights_areOn\": ";
  data += lightsAreOn() == 1 ? "true" : "false";
  
  data += " }";
  
  return data;
}

void startRelay(int pinNumber){
  digitalWrite(pinNumber, 1);
  Serial.println("pin #" + (String)pinNumber + " on");

  if(pinNumber == relayPin1){
    relayStarted[0] = micros();
  } else if (pinNumber == relayPin2){
    relayStarted[1] = micros();
  }
}

void checkRelays() {
  unsigned long now = micros();
  int period = relayBlinkPeriod * 1000; // convert micros into ms
  if(digitalRead(relayPin1) == 1 && now - relayStarted[0] > period){
    digitalWrite(relayPin1, 0);
    Serial.println("pin #" + (String)relayPin1 + " off");
  }
  if(digitalRead(relayPin2) == 1 && now - relayStarted[1] > period){
    digitalWrite(relayPin2, 0);
    Serial.println("pin #" + (String)relayPin2 + " off");
  }
}

void setup_routes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    Serial.println("Sending sensor data...");
    String data = getSensorData();
    Serial.println(data);
    request->send(200, "application/json", data);
  });
       
  server.on("/odoor1", HTTP_POST, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    if (switchStateCur1 == 0){
      Serial.println("Opening Door #1");
      startRelay(relayPin1);
    } else {
      Serial.println("Door #1 Already Open");
    }
    
    request->send(204);
  });

  server.on("/cdoor1", HTTP_POST, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    if (switchStateCur1 == 1){
      Serial.println("Closing Door #1");
      startRelay(relayPin1);
    } else {
      Serial.println("Door #1 Already Closed");
    }
    
    request->send(204);
  });
  
  server.on("/odoor2", HTTP_POST, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    if (switchStateCur2 == 0){
      Serial.println("Opening Door #2");
      startRelay(relayPin2);
    } else {
      Serial.println("Door #2 Already Open");
    }
    
    request->send(204);
  });

  server.on("/cdoor2", HTTP_POST, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    if (switchStateCur2 == 1){
      Serial.println("Closing Door #2");
      startRelay(relayPin2);
    } else {
      Serial.println("Door #2 Already Closed");
    }
    
    request->send(204);
  });
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);

  pinMode(switchPin1, INPUT_PULLUP);//setup pin as input
  pinMode(switchPin2, INPUT_PULLUP);//setup pin as input
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin1, LOW);
  digitalWrite(relayPin2, LOW);

  // Gives the serial monitor time to initialize
  // Ensures that the first few lines of output are printed to the monitor
  delay(200);
  
  // Connect to Wi-Fi
  wifi_connect();

  // Route for root / web page
  setup_routes();

  // Start server
  server.begin();
}

void loop() {
  switchStateCur1 = digitalRead(switchPin1);
  switchStateCur2 = digitalRead(switchPin2);
  checkRelays();
}
