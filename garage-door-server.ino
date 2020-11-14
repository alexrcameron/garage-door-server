#include "config.h"
#include "ClientConfig.h"

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

OneWire oneWire(14);
DallasTemperature sensors(&oneWire);

uint8_t tempSensors[][8] = {
  { 0x28, 0x67, 0x4C, 0x8F, 0x1E, 0x19, 0x01, 0x6D  },
  { 0x28, 0x76, 0x92, 0x5B, 0x1E, 0x19, 0x01, 0xF2  },
  { 0x28, 0x60, 0xE6, 0x8A, 0x1E, 0x19, 0x01, 0xE7  }
};

int tempSensorOverridePin = 0;

const char* ssid = CONFIG_NETWORK_SSID;
const char* password = CONFIG_NETWORK_PASSWORD;

const char* http_username = CONFIG_LOGIN_USERNAME;
const char* http_password = CONFIG_LOGIN_PASSWORD;

AsyncWebServer server(80);

int switchPin1 = 12;
int switchStateCur1;
int relayPin1 = 4;

int switchPin2 = 13;
int switchStateCur2;
int relayPin2 = 5;

int relayBlinkPeriod = 400; // length of led blink in ms
unsigned long relayStarted[] = {0,0};

void wifi_connect() {
  // Check if we have a WiFi connection, if we don't, connect.
  int xCnt = 0;

  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Connecting to " + (String)ssid);
  
    WiFi.mode(WIFI_STA);
    
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED  && xCnt < 50) {
      delay(500);
      Serial.print(".");
      xCnt ++;
    }

    if (WiFi.status() != WL_CONNECTED){
      Serial.println("Failed To Connect!");
    } else {
      Serial.println("WiFi Connected");
      Serial.println("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.println(WiFi.macAddress());
    }

  } else {
    Serial.println("Wifi Already Connected");
      Serial.println("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.println(WiFi.macAddress());
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

    request->send_P(200, "text/html", index_html);
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();

    Serial.println("Sending sensor data...");

    if(digitalRead(tempSensorOverridePin) != 0) {
      sensors.requestTemperatures();
    }

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
  pinMode(tempSensorOverridePin, INPUT_PULLUP);//setup pin as input
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
