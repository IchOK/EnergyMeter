#ifndef _JCA_EM_MEASURING_
#define _JCA_EM_MEASURING_

#include <Arduino.h>
#include <FreeRTOS.h>
#include <Preferences.h>
#include <cstring>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#define JCA_EM_VOLTAGE_SENSES 1
#define JCA_EM_CURRENT_SENSES 8
#define JCA_EM_SAMPLEPERIODES 2
#define JCA_EM_SAMPLES_PERPEROIDE 50
#define JCA_EM_SAMPLECOUNT (JCA_EM_SAMPLEPERIODES * JCA_EM_SAMPLES_PERPEROIDE)
#define JCA_EM_STORAGENAME "JcaEm"
#define JCA_EM_TAG_CONFIG "Conf"
#define JCA_EM_TAG_COUNTER "Cnt"
#define JCA_EM_COUNTER_SAVE_INTERVALL 36000000
#define JCA_EM_CAL_RATE 200
#define JCA_EM_CAL_TIME 1000000
#define JCA_EM_CAL_PERIODES 10
#define JCA_EM_MEAN_SAMPLES 5
#define JCA_EM_FREQ_PERIODES 10

namespace JCA {
  namespace EM {
    enum OperMode_E {
      Idle = 0,
      WaitZero = 1,
      ReadZero = 2,
      WaitCalibration = 3,
      ReadCalibration = 4,
      ReadValue = 11
    };

    struct InputConfig_T {
      int Pin;                // Pin der Analogmessung
      uint16_t Offset;             // Nullpunktverschiebung
      float Factor;           // Faktor des Wandlers
    };

    struct CounterData_T {
      uint32_t Import; // Verbrauchte Energie [Wh]
      uint32_t Export; // Erzeugte Energie [Wh]
    };

    struct DeviceConfig_T
    {
      unsigned long PeriodMicros;
      unsigned long SampleRate;
      unsigned long UpdateMillis;
      InputConfig_T VoltageInputs [JCA_EM_VOLTAGE_SENSES];
      InputConfig_T CurrentInputs [JCA_EM_CURRENT_SENSES];
      uint8_t CurrentMapping [JCA_EM_CURRENT_SENSES];
    };

    struct DeviceCounter_T {
      CounterData_T Counters[JCA_EM_CURRENT_SENSES];
    };

    struct PowerInput_T {
      bool Valid;
      struct {
        float VoltageRMS;       // Effektive Spannung [V]
        float CurrentRMS;       // Effektiver Strom [A]
        float Power;            // Scheinleistung [VA]
        float PowerActiv;       // Wirkleistung [W]
        float PowerReactiv;     // Blindleistung [var]
        float PowerFactor;      // Leistungsfaktor / CosPhi
        float PartImport;       // Verbrauchte Energie [Wh] - Kommastellen
        float PartExport;       // Erzeugte Energie [Wh] - Kommastellen
        bool FirstDone;
        unsigned long LastRead;
      } Data;
      struct {
        uint16_t Current[JCA_EM_SAMPLECOUNT];
        uint16_t Voltage[JCA_EM_SAMPLECOUNT];
        unsigned long StartRead;
      } RawData;
    };

    typedef std::function<void (JsonObject Data)> JsonObjectCallback;

    // _Calc.cpp
    int16_t getMeanValue (int16_t Samples[], uint8_t Count);
    void calcData (uint8_t _Channel);

    // _Interface.cpp
    void setCommands (JsonObject _JData, JsonObject _JCmd);
    void setConfig (JsonObject _JData, JsonObject _JCmd);
    void addConfig (JsonObject _JData);
    void addData (JsonObject _JData);
    void addDetail (JsonObject _JData, int8_t _Channel = 0);
    void getRawChannel (JsonObject _JData, int8_t _Channel);
    void addWebSocketCallback(JsonObjectCallback _CB);
    void startZeroTask (JsonObject _JData);
    void startCalibrationTask (JsonObject _JData);
    void startReadTask (JsonObject _JData);

    // _Rest.cpp
    void onRestGetConfig (AsyncWebServerRequest *_Request);
    void onRestGetData (AsyncWebServerRequest *_Request);
    void onRestGetDetail (AsyncWebServerRequest *_Request);
    void onRestPostCmd (AsyncWebServerRequest *_Request);
    void onRestPostBody (AsyncWebServerRequest *_Request, uint8_t *_Data, size_t _Len, size_t _Index, size_t _Total);
    void onWebNotFound (AsyncWebServerRequest *_Request);

    // _StoreData.cpp
    void dataRead ();
    void configSave ();
    void counterSave ();

    // _WebSocket.cpp
    void handleWebSocketMessage (void *_Arg, uint8_t *_Data, size_t _Len, JsonObject _JData);
    void onWsEvent(AsyncWebSocket * _Server, AsyncWebSocketClient * _Client, AwsEventType _Type, void * _Arg, uint8_t *_Data, size_t _Len);

    // _Tasks.cpp
    void taskGetZero (void *_TaskParameters);
    void taskCalibrate (void *_TaskParameters);
    void taskReadData (void *_TaskParameters);
    void loop ();
  }
}

#endif