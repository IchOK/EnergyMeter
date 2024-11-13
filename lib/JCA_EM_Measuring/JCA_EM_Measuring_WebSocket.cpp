#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern OperMode_E OperMode;
    extern JsonObjectCallback cbWebSocket;

    JsonDocument SocketMsgJDoc;
    String SocketMsgString;

    void handleWebSocketMessage(void *_Arg, uint8_t *_Data, size_t _Len, JsonObject _JData) {
      AwsFrameInfo *info = (AwsFrameInfo*)_Arg;
      if (info->final && info->index == 0 && info->len == _Len && info->opcode == WS_TEXT) {
        JsonDocument JCmdDoc;
        deserializeJson (JCmdDoc, _Data);
        JsonObject Cmd = JCmdDoc.to<JsonObject> ();
        getCommands (_JData, Cmd);
      }
    }

    void onWsEvent(AsyncWebSocket *_Server, AsyncWebSocketClient *_Client, AwsEventType _Type, void *_Arg, uint8_t *_Data, size_t _Len) {
      JsonObject Data;
      switch (_Type) {
        case WS_EVT_CONNECT:
          Serial.printf("WebSocket client #%u connected from %s\n", _Client->id(), _Client->remoteIP().toString().c_str());
          break;
        case WS_EVT_DISCONNECT:
          Serial.printf("WebSocket client #%u disconnected\n", _Client->id());
          break;
        case WS_EVT_DATA:
          SocketMsgJDoc.clear ();
          Data = SocketMsgJDoc["Data"].to<JsonObject> ();
          Data["mode"] = OperMode;
          handleWebSocketMessage (_Arg, _Data, _Len, Data);
          serializeJson (Data, SocketMsgString);
          _Client->text (SocketMsgString);
          break;

        case WS_EVT_PONG:
        case WS_EVT_ERROR:
          break;
      }
    }
  }
}
