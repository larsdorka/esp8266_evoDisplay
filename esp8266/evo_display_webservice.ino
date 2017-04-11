#include <ESP8266WiFi.h>
#include "SSD1306.h"
#include <ArduinoJson.h>
#include "FS.h"

#include <WiFiUdp.h>

#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <AsyncWebSocket.h>
#include "ESPAsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include <SPIFFSEditor.h>
#include <StringArray.h>
#include <WebAuthentication.h>
#include <WebHandlerImpl.h>
#include <WebResponseImpl.h>

#include "logo.h"

#define CONN_LED D0
#define SCL D1
#define SDA D2

const char fileName[] = "/config/wlan.json";

SSD1306 display(0x3c, SDA, SCL);

enum apiMethods {
  none,
  setDisplay,
  setColorAndText
};
apiMethods processSwitch = none;

unsigned long l_blinktime = 0;
unsigned long l_connTime = 0;
unsigned long l_lcdTime = 0;
WiFiUDP udp;
int i_udpPort = 9999;
int i_packetSize = 0;
char data[200];
int i_wwwPort = 80;
String s_body = "";
String displayLog[4] = {"", "", "", ""};
boolean b_connected = false;
boolean b_ledState = false;

AsyncWebServer server(i_wwwPort);


void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
  if (!index) Serial.printf("BodyStart: %u\n", total);
  Serial.printf("%s", (const char*)data);
  if (index + len == total) Serial.printf("\nBodyEnd: %u\n", total);
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if (!index) Serial.printf("UploadStart: %s\n", filename.c_str());
  Serial.printf("%s", (const char*)data);
  if (final) Serial.printf("\nUploadEnd: %s (%u)\n", filename.c_str(), index+len);
}

String getColorFromIP(IPAddress address, int port) {
  String s_colors = "GGGGGGG";
  for (int i=0; i<4; i++) {
    int len = String(address[i], DEC).length();
    for (int c=0; c<len; c++) {
      s_colors += "R";
    }
    s_colors += "O";
  }
  if (port != 80) {
    int len = String(port, DEC).length();
    for (int p=0; p<len; p++) {
     s_colors += "R";
    }
  }
  return s_colors;
}

void refreshLCD() {
  if (millis() > l_lcdTime + 250) {
    display.clear();
    setLCD();
    display.display();
    l_lcdTime = millis();
  }
}

void setLCD() {
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "UDP: " + WiFi.localIP().toString() + ":" + String(i_udpPort));
  for (int i=0; i<4; i++) {
    display.drawString(0,i * 10 + 20, displayLog[i]);
  }
}

void writeToDisplay(String s_text, boolean b_log) {
  if (b_log) writeToLog(s_text);
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

void processApiSetColorAndText(String parseBody) {
  Serial.println("SetColorAndText: " + parseBody);
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(parseBody);
  String s_color = String((const char*)root["color"]);
  writeToDisplay(s_color, true);
  String s_text = String((const char*)root["text"]);
  writeToDisplay(s_text, true);
}

void processApiSetDisplay(String parseBody) {
  Serial.println("SetDisplay: " + parseBody);
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(parseBody);
  String s_textMode = String((const char*)root["text"]["mode"]);
  String s_text = "0A" + s_textMode;
  String s_textContent = String((const char*)root["text"]["content"]);
  s_text += s_textContent;
  String s_colorMode = String((const char*)root["color"]["mode"]);
  String s_colorContent = "";
  if (s_colorMode.equalsIgnoreCase("content")) {
    s_colorContent = String((const char*)root["color"]["content"]);
  } else if (s_colorMode.equalsIgnoreCase("red")) {
    for (int i=0; i<s_textContent.length(); i++) s_colorContent += "R";
  } else if (s_colorMode.equalsIgnoreCase("orange")) {
    for (int i=0; i<s_textContent.length(); i++) s_colorContent += "O";
  } else if (s_colorMode.equalsIgnoreCase("green")) {
    for (int i=0; i<s_textContent.length(); i++) s_colorContent += "G";
  } else {
    s_colorContent = String((const char*)root["color"]["content"]);
  }
  String s_color = "0E" + s_colorContent;
  writeToDisplay(s_color, true);
  writeToDisplay(s_text, true);
}

void processApiClear() {
  String s_text = "0A0                              ";
  writeToDisplay(s_text, false);
}


void setup() {
  // put your setup code here, to run once: 
  pinMode(CONN_LED, OUTPUT);
  digitalWrite(CONN_LED, HIGH);
  
  Serial.begin(9600);
  Serial.println();
  
  Serial1.begin(9600);

  writeToDisplay("0EGGGGGGGGGGGGGGGGGGGG", false);
  writeToDisplay("0A1initializing...", false);
  
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  display.drawXbm(0, 0, logo_width, logo_height, logo);
  display.display();
  delay(1000);

  String fileBuffer = "";
  StaticJsonBuffer<512> jsonBuffer;
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
    setLCD();
    display.display();
    String colors = "0E" + getColorFromIP(WiFi.localIP(), i_wwwPort);
    writeToDisplay(colors, false);
    writeToDisplay("0A0HTTP://" + WiFi.localIP().toString(), false);
    
    udp.begin(i_udpPort);
  }
  else {
    writeToDisplay("0A5Error: no network connection", false);
    display.clear();
    display.drawString(0, 0, "Error: could not connect to any network");
    display.display();
  }

  server.on("/api/clear", HTTP_ANY, [](AsyncWebServerRequest *request){
    Serial.println("serving: api/clear");
    request->send(200);
    processApiClear();
  });

  server.on("/api/setcolorandtext", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("serving: api/setcolorandtext");
    request->send(200);
  }, onUpload, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if (!index) s_body = "";
    s_body += String((const char*)data);
    if (index + len == total) processSwitch = setColorAndText;
  });
  
  server.on("/api/setdisplay", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("serving: api/setdisplay");
    request->send(200);
  }, onUpload, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if (!index) s_body = "";
    s_body += String((const char*)data);
    if (index + len == total) processSwitch = setDisplay;
  });

  server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/api-doc/displayApi.json", "text/json");
  });

  server.serveStatic("/", SPIFFS, "/www").setDefaultFile("index.html");

  server.addHandler(new SPIFFSEditor());
  
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/www/404.html");
  });

  server.onFileUpload(onUpload);

  server.onRequestBody(onBody);
  
  server.begin();
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
      String s_display = "";
      s_display = String(data);
      writeToDisplay(s_display, true);
    }
  }
  switch(processSwitch)
  {
    case none:
      break;
    case setDisplay:
      processApiSetDisplay(s_body);
      processSwitch = none;
      break;
    case setColorAndText:
      processApiSetColorAndText(s_body);
      processSwitch = none;
      break;
    default:
      processSwitch = none;
      break;
  }
  digitalWrite(CONN_LED, !b_ledState);
  refreshLCD();
}


