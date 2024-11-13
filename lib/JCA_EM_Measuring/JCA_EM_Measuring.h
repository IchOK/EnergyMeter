#ifndef _JCA_EM_MEASURING_
#define _JCA_EM_MEASURING_

#include <Arduino.h>
#include <FreeRTOS.h>
#include "LittleFS.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define JCA_EM_CALFILE "/Calibration.json"
#define JCA_EM_MAX_CURRENT 8
#define JCA_EM_SAMPLEPERIODES 5
// 50Hz
#define JCA_EM_PERIODE_US 20000
#define JCA_EM_SAMPLES 40
// 60Hz
// #define JCA_EM_PERIODE_US 16666
// #define JCA_EM_SAMPLES 20

namespace JCA {
  namespace EM {
    enum OperMode_E {
      Idle = 0,
      ReadZero = 1,
      DoneZero = 2,
      ReadCalibration = 3,
      DoneCalibration = 4,
      ReadValue = 11,
      ReadRaw = 12
    };

    struct InputConfig {
      int Pin;                // Pin der Analogmessung
      int Offset;             // Nullpunktverschiebung
      float Factor;           // Faktor des Wandlers
    };

    struct PowerInput {
      InputConfig Config;
      struct {
        float VoltageRMS;       // Effektive Spannung [V]
        float CurrentRMS;       // Effektiver Strom [A]
        float Power;            // Scheinleistung [VA]
        float PowerActiv;       // Wirkleistung [W]
        float PowerReactiv;     // Blindleistung [var]
        float PowerFactor;      // Leistungsfaktor / CosPhi
        bool Done;
      } Data;
      struct {
        float PartImport;        // Zähler der Kommastellen [Wh]
        uint32_t Import;          // Zähler der Wirkenergie
        float PartExport;
        uint32_t Export;
      } Counter;
      struct {
        int Current[JCA_EM_SAMPLEPERIODES * JCA_EM_SAMPLES + 1];
        int Voltage[JCA_EM_SAMPLEPERIODES * JCA_EM_SAMPLES + 1];
        uint32_t TimeMicros[JCA_EM_SAMPLEPERIODES * JCA_EM_SAMPLES + 1];
        uint16_t Counter;
        uint32_t StartRead;
        uint32_t LastRead;
        bool Done;
        bool FirstDone;
      } RawData;
    };

    typedef std::function<void (JsonObject Data)> JsonObjectCallback;

    // _Calc.cpp
    void calcSaveZero (JsonObject _JData);
    void calcSaveCalibration (JsonObject _JData);
    void calcData ();

    // _File.cpp
    void fileRead ();
    void fileSave ();

    // _Interface.cpp
    void getCommands (JsonObject _JData, JsonObject _JCmd);
    void addData (JsonObject _JData);
    void addDetail (JsonObject _JData, int8_t _Channel = 0);
    void getRawChannel (JsonObject _JData, int8_t _Channel);
    void addWebSocketCallback(JsonObjectCallback _CB);
    void definePins(int _PinVoltage, int _PinsCurrent[JCA_EM_MAX_CURRENT]);

    // _Rest.cpp
    void onRestGetData (AsyncWebServerRequest *_Request);
    void onRestGetDetail (AsyncWebServerRequest *_Request);
    void onRestPostCmd (AsyncWebServerRequest *_Request);
    void onWebNotFound (AsyncWebServerRequest *_Request);

    // _WebSocket.cpp
    void handleWebSocketMessage (void *_Arg, uint8_t *_Data, size_t _Len);
    void onWsEvent(AsyncWebSocket * _Server, AsyncWebSocketClient * _Client, AwsEventType _Type, void * _Arg, uint8_t *_Data, size_t _Len);

    // _Tasks.cpp
    void taskReadData ( void *_TaskParameters);
    void taskAnalysData (void *_TaskParameters);
  }
}

#endif