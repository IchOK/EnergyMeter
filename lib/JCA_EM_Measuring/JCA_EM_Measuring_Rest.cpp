#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern OperMode_E OperMode;

    JsonDocument RestMsgJDoc;
    String RestMsgString;

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
      Data["mode"] = OperMode;

      JsonObject Cmd = RestMsgJDoc["Cmd"].to<JsonObject> ();

      for (int i = 0; i < _Request->params(); i++) {
        Cmd[_Request->getParam(i)->name()] = _Request->getParam(i)->value();
      }

      getCommands (Data, Cmd);
      
      serializeJson (Data, RestMsgString);
      _Request->send (200, "application/json", RestMsgString);
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
