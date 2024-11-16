#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern OperMode_E OperMode;

    JsonDocument RestMsgJDoc;
    String RestMsgString;

    void onRestGetConfig(AsyncWebServerRequest *_Request) {
      RestMsgJDoc.clear();
      JsonObject Data = RestMsgJDoc["Data"].to<JsonObject> ();
      Data["mode"] = OperMode;

      Data["type"] = "GetConfig";
      addConfig (Data);
      serializeJson (Data, RestMsgString);
      _Request->send (200, "application/json", RestMsgString);
    }

    void onRestGetData(AsyncWebServerRequest *_Request) {
      RestMsgJDoc.clear();
      JsonObject Data = RestMsgJDoc["Data"].to<JsonObject> ();
      Data["mode"] = OperMode;

      Data["type"] = "GetData";
      addData (Data);
      serializeJson (Data, RestMsgString);
      _Request->send (200, "application/json", RestMsgString);
    }

    void onRestGetDetail(AsyncWebServerRequest *_Request) {
      RestMsgJDoc.clear();
      JsonObject Data = RestMsgJDoc["Data"].to<JsonObject> ();
      Data["mode"] = OperMode;

      if(_Request->hasParam("input")){
          Data["type"] = "GetDetail";
          addDetail (Data, _Request->getParam ("input")->value ().toInt ());
      } else {
        Data["type"] = "Error";
        Data["message"] = "Not input selected";
      }
      serializeJson (Data, RestMsgString);
      _Request->send (200, "application/json", RestMsgString);
    }

    void onRestPostCmd(AsyncWebServerRequest *_Request) {
      RestMsgJDoc.clear();
      JsonObject Data = RestMsgJDoc["Data"].to<JsonObject> ();
      JsonObject Cmd = RestMsgJDoc["Cmd"].to<JsonObject> ();
      Data["mode"] = OperMode;

      // Commandos auf Argumenten auslesen
      for (int i = 0; i < _Request->params(); i++) {
        Cmd[_Request->getParam(i)->name()] = _Request->getParam(i)->value();
      }
      setCommands (Data, Cmd);

      // Konfiguration aus Body-Daten auslasen, falls vorhanden
      if (_Request->_tempObject != nullptr) {
        JsonDocument JBuffer;
        DeserializationError Error = deserializeJson (JBuffer, (char *)(_Request->_tempObject));
        if (!Error) {
          setConfig(Data, JBuffer.as<JsonObject>());
        }
        
        free(_Request->_tempObject);
        _Request->_tempObject = nullptr;
      }
      
      serializeJson (Data, RestMsgString);
      _Request->send (200, "application/json", RestMsgString);
    }

    void onRestPostBody (AsyncWebServerRequest *_Request, uint8_t *_Data, size_t _Len, size_t _Index, size_t _Total) {
      if (_Total > 0 && _Request->_tempObject == nullptr) {
        _Request->_tempObject = malloc (_Total + 10);
      }
      if (_Request->_tempObject != nullptr) {
        memcpy ((uint8_t *)(_Request->_tempObject) + _Index, _Data, _Len);
      }
    }

    void onWebNotFound(AsyncWebServerRequest *_Request) {
      RestMsgJDoc.clear ();
      JsonObject Data = RestMsgJDoc["Data"].to<JsonObject> ();
      Data["type"] = "Error";
      Data["message"] = "Endpoint not defined";
      serializeJson (Data, RestMsgString);
      _Request->send (404, "application/json", RestMsgString);
    }
  }
}
