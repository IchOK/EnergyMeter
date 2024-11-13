#include <Arduino.h>
#include <FreeRTOS.h>
#include "LittleFS.h"

// ##############################################################################################################
// WiFi Manager
// ##############################################################################################################
#include <WiFiManager.h>
#define WM_TRIGGER 0
#define WM_STATE BUILTIN_LED
WiFiManager WlanManager;
enum PortalState_T
{
  OFF,
  INIT,
  ON
} PortalState;
bool TriggerDone;
const char *ssid = "EnergieTestAP";
const char *password = "EnergieTestPass";

// ##############################################################################################################
// ASync WebServer  -  RestAPI / WebSocket
// ##############################################################################################################
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <JCA_EM_Measuring.h>
AsyncWebServer ApiServer (81);
AsyncWebSocket WebSocket("/ws");
TaskHandle_t HandleReadData;
TaskHandle_t HandleAnalysData;

void UpdateCallback(JsonObject Data) {
  if (WebSocket.count() > 0) {
    String Message;
    serializeJson(Data, Message);
    WebSocket.textAll(Message);
  }
}

void onRestGetTasks(AsyncWebServerRequest *request) {
  JsonDocument JDoc;
  String MessageString;
  JDoc["TaskRead"] = eTaskGetState (HandleReadData);
  JDoc["TaskAnalyse"] = eTaskGetState (HandleAnalysData);

  serializeJson (JDoc, MessageString);
  request->send (200, "application/json", MessageString);
}


// ##############################################################################################################
// SETUP
// ##############################################################################################################
void setup (){
  WiFi.mode (WIFI_STA);
  delay (1000);
  Serial.begin (115200);
  Serial.println ("Startup)");

  //----------------------------
  // Filesystem
  if(!LittleFS.begin(true)){
    Serial.println("An Error has occurred while mounting LittleFS");
  }

  //----------------------------
  // WiFi Manager
  pinMode (WM_STATE, OUTPUT);
  pinMode (WM_TRIGGER, INPUT_PULLUP);
  WlanManager.setDarkMode (true);
  WlanManager.setConfigPortalBlocking (false);
  WlanManager.setAPStaticIPConfig (IPAddress (192, 168, 1, 1), IPAddress (0, 0, 0, 0), IPAddress (255, 255, 255, 0));
  if (WlanManager.autoConnect (ssid, password)) {
    Serial.println ("Connection DONE");
    PortalState = OFF;
  } else {
    Serial.println ("Connection FAILED - Configportal running");
    PortalState = INIT;
  }

  //----------------------------
  // WebSocket
  WebSocket.onEvent(JCA::EM::onWsEvent);
  ApiServer.addHandler(&WebSocket);
  //----------------------------
  // WebServer
  ApiServer.on ("/api/data", HTTP_GET, JCA::EM::onRestGetData);
  ApiServer.on ("/api/detail", HTTP_GET, JCA::EM::onRestGetDetail);
  ApiServer.on ("/api/tasks", HTTP_GET, onRestGetTasks);
  ApiServer.on ("/api/cmd", HTTP_POST, JCA::EM::onRestPostCmd);
  ApiServer.onNotFound (JCA::EM::onWebNotFound);
  ApiServer.begin ();

  //----------------------------
  // EnergyMeter
  int CurrentPins[JCA_EM_MAX_CURRENT] = {9,5,6,4,7,3,1,2};
  JCA::EM::definePins(8, CurrentPins);
  JCA::EM::addWebSocketCallback(UpdateCallback);

  //----------------------------
  // Tasks
  xTaskCreatePinnedToCore(
    JCA::EM::taskReadData,
    "ReadData",
    10000,
    NULL,
    1000,
    &HandleReadData,
    1);
  xTaskCreatePinnedToCore(
    JCA::EM::taskAnalysData,
    "ShowData",
    10000,
    NULL,
    1000,
    &HandleAnalysData,
    0);

//  char Buffer[20000];
//  vTaskList(Buffer);
//  Serial.println("TASK LIST");
//  Serial.println(Buffer);

  Serial.print ("Tasks:");
  Serial.println (uxTaskGetNumberOfTasks ());
}

// ##############################################################################################################
// LOOP
// ##############################################################################################################
void loop () {
  // WLAN-Manager
  digitalWrite (WM_STATE, PortalState != OFF);
  if (PortalState == INIT) {
    WlanManager.process ();
    if (!WlanManager.getConfigPortalActive ()) {
      PortalState = OFF;
    }
  } else if (PortalState == ON) {
    WlanManager.process ();
    if (!WlanManager.getWebPortalActive ()) {
      PortalState = OFF;
    }
    if (digitalRead (WM_TRIGGER) == LOW && !TriggerDone) {
      delay (50);
      if (digitalRead (WM_TRIGGER) == LOW) {
        TriggerDone = true;
        Serial.println ("Button Pressed, Stopping Portal");
        WlanManager.stopWebPortal ();
        PortalState = OFF;
      }
    }
  } else if (PortalState == OFF) {
    if (digitalRead (WM_TRIGGER) == LOW && !TriggerDone) {
      delay (50);
      if (digitalRead (WM_TRIGGER) == LOW) {
        TriggerDone = true;
        Serial.println ("Button Pressed, Starting Portal");
        WlanManager.startWebPortal ();
        PortalState = ON;
      }
    }
  }
  if (TriggerDone) {
    if (digitalRead (WM_TRIGGER) == HIGH) {
      TriggerDone = false;
    }
  }
}
