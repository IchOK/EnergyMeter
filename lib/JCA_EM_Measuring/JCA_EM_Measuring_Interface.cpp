#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern InputConfig VoltageInput;
    extern PowerInput CurrentInputs[JCA_EM_MAX_CURRENT];
    extern OperMode_E OperMode;
    extern bool ZeroDone;
    extern bool CalDone;
    extern float Cal_VoltageRMS;
    extern float Cal_CurrentRMS;
    extern JsonObjectCallback cbWebSocket;

    void getCommands (JsonObject _JData, JsonObject _JCmd) {
      serializeJson(_JCmd, Serial);
      Serial.println();
      if (_JCmd["Cmd"].is<String>()) {
        String Cmd = _JCmd["Cmd"].as<String>();
        if (Cmd == "ZeroStart") {
          Serial.println ("Start Read-Zero");
          _JData["type"] = "CmdZeroStart";

          OperMode = OperMode_E::ReadZero;
        }
        if (Cmd == "ZeroSave") {
          if (OperMode == OperMode_E::DoneZero) {
            Serial.println ("Save-Zero");
            _JData["type"] = "CmdZeroSave";

            calcSaveZero (_JData);
          } else {
            _JData["type"] = "Error";
            _JData["message"] = "Zero measurment not done";
          }
        }
        if (Cmd == "CalStart") {
          if (ZeroDone) {
            Serial.println ("Start Calibration-Zero");

            if (!_JCmd["Voltage"].is<String>()){
              _JData["type"] = "Error";
              _JData["message"] = "Cal Voltage missing";
              _JData[Cmd] = _JCmd;
            } else if (!_JCmd["Current"].is<String>()) {
              _JData["type"] = "Error";
              _JData["message"] = "Cal Current missing";
              _JData[Cmd] = _JCmd;
            } else {
              _JData["type"] = "CmdCalStart";
              _JData["message"] = "CmdCalStart";
              Cal_VoltageRMS = _JCmd["Voltage"].as<float> ();
              Cal_CurrentRMS = _JCmd["Current"].as<float> ();
              
              OperMode = OperMode_E::ReadCalibration;
            }
          } else {
            _JData["type"] = "Error";
            _JData["message"] = "Zero offset not set";
          }
        }
        if (Cmd == "CalSave" && OperMode == OperMode_E::DoneCalibration) {
          Serial.println ("Cal-Zero");
          _JData["type"] = "CmdCalCave";

          calcSaveCalibration (_JData);
        }
        if (Cmd == "ReadValue" && CalDone) {
          Serial.println ("Start reading Values");
          _JData["type"] = "CmdZeroStart";

          OperMode = OperMode_E::ReadValue;
        }
        if (Cmd == "ReadRaw") {
          Serial.println ("Start reading Raw");
          _JData["type"] = "CmdZeroStart";

          OperMode = OperMode_E::ReadRaw;
        }
      }
    }

    void addData(JsonObject _JData) {
      _JData["type"] = "Data";
      for (int i = 0; i < JCA_EM_MAX_CURRENT; i++) {
        if (CurrentInputs[i].Data.Done) {
          JsonObject ChannelObj = _JData["Channel_" + String (i + 1)].to<JsonObject>();
          ChannelObj["VoltageRMS"] = CurrentInputs[i].Data.VoltageRMS;
          ChannelObj["CurrentRMS"] = CurrentInputs[i].Data.CurrentRMS;
          ChannelObj["Power"] = CurrentInputs[i].Data.Power;
          ChannelObj["PowerActiv"] = CurrentInputs[i].Data.PowerActiv;
          ChannelObj["PowerReactiv"] = CurrentInputs[i].Data.PowerReactiv;
          ChannelObj["PowerFactor"] = CurrentInputs[i].Data.PowerFactor;

          ChannelObj["Import"] = CurrentInputs[i].Counter.Import;
          ChannelObj["Export"] = CurrentInputs[i].Counter.Export;

          ChannelObj["Counter"] = CurrentInputs[i].RawData.Counter;

          CurrentInputs[i].Data.Done = false;
        }
      }
    }

    void addDetail(JsonObject _JData, int8_t _Channel) {
      if (_Channel == 0) {
        _JData["type"] = "Raw";
        for (int i = 0; i < JCA_EM_MAX_CURRENT; i++) {
          if (CurrentInputs[i].RawData.Done) {
            getRawChannel(_JData, i);
            CurrentInputs[i].RawData.Done = false;
          }
        }
      } else {
        if (_Channel > 0 && _Channel <= JCA_EM_MAX_CURRENT) {
          _JData["type"] = "Raw";
          int i = _Channel - 1;
          if (CurrentInputs[i].RawData.Done) {
            getRawChannel(_JData, i);
            CurrentInputs[i].RawData.Done = false;
          }
        } else {
          _JData["type"] = "Error";
          _JData["message"] = "Channel out of Range";
        }
      }
    }

    void getRawChannel(JsonObject _JData, int8_t _Channel) {
      if (CurrentInputs[_Channel].RawData.Done && CurrentInputs[_Channel].Config.Pin > 0) {
        JsonObject ChannelObj = _JData["Channel_" + String (_Channel + 1)].to<JsonObject>();
        ChannelObj["counter"] = CurrentInputs[_Channel].RawData.Counter;
        JsonArray ArrVoltage = ChannelObj["voltage"].to<JsonArray>();
        JsonArray ArrCurrent = ChannelObj["current"].to<JsonArray> ();
        JsonArray ArrMicros = ChannelObj["Micros"].to<JsonArray> ();
        for (int x = 0; x < CurrentInputs[_Channel].RawData.Counter; x++) {
          ArrVoltage.add(CurrentInputs[_Channel].RawData.Voltage[x]);
          ArrCurrent.add(CurrentInputs[_Channel].RawData.Current[x]);
          ArrMicros.add(CurrentInputs[_Channel].RawData.TimeMicros[x]);
        }
      }
    }

    void addWebSocketCallback (JsonObjectCallback _CB) {
      cbWebSocket = _CB;
    }

    void definePins(int _PinVoltage, int _PinsCurrent[JCA_EM_MAX_CURRENT]) {
      VoltageInput.Pin = _PinVoltage;
      for (int i = 0; i < JCA_EM_MAX_CURRENT; i++) {
        CurrentInputs[i].Config.Pin = _PinsCurrent[i];
      }
    }
  }
}
