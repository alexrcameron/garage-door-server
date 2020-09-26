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

float tempSensor1, tempSensor2, tempSensor3;

uint8_t sensor1[8] = { 0x28, 0x67, 0x4C, 0x8F, 0x1E, 0x19, 0x01, 0x6D  };
//uint8_t sensor1[8] = { 0x28, 0xA1, 0x02, 0x72, 0x1F, 0x19, 0x01, 0x4C  };
uint8_t sensor2[8] = { 0x28, 0x76, 0x92, 0x5B, 0x1E, 0x19, 0x01, 0xF2  };
uint8_t sensor3[8] = { 0x28, 0x60, 0xE6, 0x8A, 0x1E, 0x19, 0x01, 0xE7  };

const char* ssid = "ssid";
const char* password = "password";

const char* http_username = "username";
const char* http_password = "password";

const char* PARAM_INPUT_1 = "state";

const int output = 2;

AsyncWebServer server(80);

int switchPin = 12;//0
int switchStateCur;
int relayPin = 4;//3

int switchPin2 = 13;//0
int switchStateCur2;
int relayPin2 = 5;//3


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

String getSensorData() {
  String data = "";

  data += "{ \"door1_isOpen\": ";
  data += switchStateCur==1 ? "true" : "false";

  data += ", \"door2_isOpen\": ";
  data += switchStateCur2==1 ? "true" : "false";

  data += " }";
  
  return data;
}

void setup_routes() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
//  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
//    if(!request->authenticate(http_username, http_password))
//      return request->requestAuthentication();
//
//    String inputMessage;
//    String inputParam;
//    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
//    if (request->hasParam(PARAM_INPUT_1)) {
//      inputMessage = request->getParam(PARAM_INPUT_1)->value();
//      inputParam = PARAM_INPUT_1;
//      digitalWrite(output, inputMessage.toInt());
//    }
//    else {
//      inputMessage = "No message sent";
//      inputParam = "none";
//    }
//    Serial.println(inputMessage);
//    request->send(200, "text/plain", "OK");
//  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending sensor data...");
    String data = getSensorData();
    Serial.println(data);
    request->send(200, "application/json", data);
  });
       
  server.on("/odoor1", HTTP_POST, [](AsyncWebServerRequest *request){
    if (switchStateCur==0){
      Serial.println("Opening Door #1");
      digitalWrite(relayPin, 1);
      delay(400);
      digitalWrite(relayPin, 0);
    } else {
      Serial.println("Door #1 Already Open");
    }
    
    request->send(204);
  });

  server.on("/cdoor1", HTTP_POST, [](AsyncWebServerRequest *request){
    if (switchStateCur==1){
      Serial.println("Closing Door #1");
      digitalWrite(relayPin, 1);
      delay(400);
      digitalWrite(relayPin, 0);
    } else {
      Serial.println("Door #1 Already Closed");
    }
    
    request->send(204);
  });
   
  server.on("/odoor2", HTTP_POST, [](AsyncWebServerRequest *request){
    if (switchStateCur2==0){
      Serial.println("Opening Door #2");
      digitalWrite(relayPin2, 1);
      delay(400);
      digitalWrite(relayPin2, 0);
    } else {
      Serial.println("Door #2 Already Open");
    }
    
    request->send(204);
  });

  server.on("/cdoor2", HTTP_POST, [](AsyncWebServerRequest *request){
    if (switchStateCur2==1){
      Serial.println("Closing Door #2");
      digitalWrite(relayPin2, 1);
      delay(400);
      digitalWrite(relayPin2, 0);
    } else {
      Serial.println("Door #2 Already Closed");
    }
    
    request->send(204);
  });
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);

  pinMode(switchPin, INPUT_PULLUP);//setup pin as input
  pinMode(switchPin2, INPUT_PULLUP);//setup pin as input
  pinMode(relayPin, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  digitalWrite(relayPin, LOW);
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
  switchStateCur = digitalRead(switchPin);
  switchStateCur2 = digitalRead(switchPin2);
  //server.handleClient();
}
