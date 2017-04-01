#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "SSD1306.h"
#include <ArduinoJson.h>
#include "FS.h"

#include "logo.h"

#define CONN_LED D0
#define SCL D1
#define SDA D2

const char fileName[] = "/wlan.json";
String fileBuffer = "";
StaticJsonBuffer<200> jsonBuffer;

SSD1306 display(0x3c, SDA, SCL);

unsigned long l_blinktime = 0;
unsigned long l_connTime = 0;
unsigned long l_displayTime = 0;
WiFiUDP udp;
int i_port = 9999;
int i_packetSize = 0;
char data[200];
String s_display = "";
String displayLog[4] = {"", "", "", ""};
boolean b_connected = false;
boolean b_ledState = false;


void setup() {
  // put your setup code here, to run once: 
  pinMode(CONN_LED, OUTPUT);
  digitalWrite(CONN_LED, HIGH);
  Serial1.begin(9600);
  
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  display.drawXbm(0, 0, logo_width, logo_height, logo);
  display.display();
  delay(1000);
  
  if (SPIFFS.begin()) {
    if (SPIFFS.exists(fileName))
    {
        File f = SPIFFS.open(fileName, "r");
        fileBuffer = f.readString();
        f.close();
    }
  }

  JsonObject& root = jsonBuffer.parseObject(fileBuffer.c_str());
  JsonArray& json_ssid = root["ssid"].asArray();
  JsonArray& json_wpa = root["wpa"].asArray();
  int wlanCount = 0;
  if (json_ssid.size() <= json_wpa.size()) {
    wlanCount = json_ssid.size();
  }
  else {
    wlanCount = json_wpa.size();
  }

  String s_connecting = "";

  while (!b_connected) {
    for (int i=0; i<wlanCount; i++) {
      unsigned long l_connStart = millis();
      unsigned long l_connTime = 0;
      WiFi.begin(json_ssid.get<String>(i).c_str(), json_wpa.get<String>(i).c_str());
      while (WiFi.status() != WL_CONNECTED) {
        l_connTime = millis() - l_connStart;
        if (l_connTime > 10000) break;
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_10);
        display.drawStringMaxWidth(64, 0, 128, "connecting to");
        display.setFont(ArialMT_Plain_16);
        display.drawStringMaxWidth(64, 10, 128, json_ssid.get<String>(i));
        display.drawProgressBar(0, 30, 120, 10, l_connTime / 100);
        display.display();
        delay(100);
      }
      b_connected = (WiFi.status() == WL_CONNECTED);
      if (b_connected) break;
    }
  }
  if (b_connected) {
    display.clear();
    setDisplay();
    display.display();
    writeToDisplay("0ERRRRRRRRRRRRRRRRRRRR");
    writeToDisplay("0A0" + WiFi.localIP().toString() + ":" + String(i_port));
    udp.begin(i_port);
  }
  else {
    writeToDisplay("0A1Error: no network connection");
    display.clear();
    display.drawString(0, 0, "Error: could not connect to any network");
    display.display();
  }
}


void loop() {
  // put your main code here, to run repeatedly:
  b_ledState = (millis() > l_blinktime + 100) && b_connected;
  i_packetSize = udp.parsePacket();
  if (i_packetSize > 0)
  {
    if (b_ledState) l_blinktime = millis();
    //Serial.printf("Received %d bytes from %s\n", i_packetSize, udp.remoteIP().toString().c_str());
    memset(data, 0, sizeof(data));
    int len = udp.read(data, 200);
    if (len > 3)
    {
      s_display = String(data);
      writeToDisplay(s_display);
      writeToLog(s_display);
    }
  }
  digitalWrite(CONN_LED, !b_ledState);
  refreshDisplay();
}


void refreshDisplay() {
  if (millis() > l_displayTime + 250) {
    display.clear();
    setDisplay();
    display.display();
    l_displayTime = millis();
  }
}


void setDisplay() {
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "UDP: " + WiFi.localIP().toString() + ":" + String(i_port));
  for (int i=0; i<4; i++) {
    display.drawString(0,i * 10 + 20, displayLog[i]);
  }
}


void writeToDisplay(String s_text) {
  Serial1.write(27);
  Serial1.print(s_text);
  Serial1.write(13);
}


void writeToLog(String s_text) {
  displayLog[3] = displayLog[2];
  displayLog[2] = displayLog[1];
  displayLog[1] = displayLog[0];
  displayLog[0] = s_text;
}

