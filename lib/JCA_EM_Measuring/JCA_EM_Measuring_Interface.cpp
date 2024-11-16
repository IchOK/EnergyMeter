#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern DeviceConfig_T DeviceConfig;
    extern DeviceCounter_T DeviceCounter;
    extern PowerInput_T PowerInputs[JCA_EM_CURRENT_SENSES];
    extern OperMode_E OperMode;
    extern bool InitDone;
    extern bool CalDone;
    extern float Cal_VoltageRMS;
    extern float Cal_CurrentRMS;
    extern JsonObjectCallback cbWebSocket;
    TaskHandle_t HandleReadData;

    void setCommands (JsonObject _JData, JsonObject _JCmd) {
      serializeJson(_JCmd, Serial);
      Serial.println();
      if (_JCmd["Cmd"].is<String>()) {
        String Cmd = _JCmd["Cmd"].as<String>();
        if (Cmd == "ZeroStart") {
          Serial.println ("Start Read-Zero");
          _JData["type"] = "CmdZeroStart";

          startZeroTask (_JData);
        }
        if (Cmd == "CalStart") {
          if (InitDone) {
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

              startCalibrationTask (_JData);
            }
          } else {
            _JData["type"] = "Error";
            _JData["message"] = "Zero offset not set";
          }
        }
      }
    }

    void setConfig (JsonObject _JData, JsonObject _JConfig) {
      bool DoSave = false;
      serializeJson(_JConfig, Serial);
      Serial.println();
      if (_JConfig["period"].is<uint32_t>()) {
        if (_JConfig["period"].as<uint32_t> () != DeviceConfig.PeriodMicros) {
          DeviceConfig.PeriodMicros = _JConfig["period"].as<uint32_t> ();
          DoSave = true;
        }
        _JData["period"] = DeviceConfig.PeriodMicros;
      }
      if (_JConfig["samplerate"].is<uint32_t>()) {
        if (_JConfig["samplerate"].as<uint32_t> () != DeviceConfig.SampleRate) {
          DeviceConfig.SampleRate = _JConfig["samplerate"].as<uint32_t> ();
          DoSave = true;
        }
        _JData["period"] = DeviceConfig.PeriodMicros;
      }
      if (_JConfig["update"].is<uint32_t>()) {
        if (_JConfig["update"].as<uint32_t> () != DeviceConfig.UpdateMillis) {
          DeviceConfig.UpdateMillis = _JConfig["update"].as<uint32_t> ();
          DoSave = true;
        }
        _JData["update"] = DeviceConfig.UpdateMillis;
      }
      if (_JConfig["current"].is<JsonArray>()) {
        JsonArray Pins = _JConfig["current"].as<JsonArray>();
        if (Pins.size () == JCA_EM_CURRENT_SENSES) {
          for (int i = 0; i < JCA_EM_CURRENT_SENSES; i++) {
            if (Pins[i].as<uint8_t>() != DeviceConfig.CurrentInputs[i].Pin) {
              DeviceConfig.CurrentInputs[i].Pin = Pins[i].as<uint8_t>();
              DoSave = true;
            }
          }
        }
        _JData["current"] = Pins;
      }
      if (_JConfig["voltage"].is<JsonArray>()) {
        JsonArray Pins = _JConfig["voltage"].as<JsonArray>();
        if (Pins.size () == JCA_EM_VOLTAGE_SENSES) {
          for (int i = 0; i < JCA_EM_VOLTAGE_SENSES; i++) {
            if (Pins[i].as<uint8_t>() != DeviceConfig.VoltageInputs[i].Pin) {
              DeviceConfig.VoltageInputs[i].Pin = Pins[i].as<uint8_t>();
              DoSave = true;
            }
          }
        }
        _JData["voltage"] = Pins;
      }
      if (_JConfig["map"].is<JsonArray>()) {
        JsonArray Pins = _JConfig["map"].as<JsonArray>();
        if (Pins.size () == JCA_EM_CURRENT_SENSES) {
          for (int i = 0; i < JCA_EM_CURRENT_SENSES; i++) {
            if (Pins[i].as<uint8_t>() != DeviceConfig.CurrentMapping[i]) {
              DeviceConfig.CurrentMapping[i] = Pins[i].as<uint8_t>();
              DoSave = true;
            }
          }
        }
        _JData["map"] = Pins;
      }

      if (DoSave) {
        configSave();
      }
    }

    void addConfig(JsonObject _JData) {
      _JData["type"] = "Config";
      _JData["period"] = DeviceConfig.PeriodMicros;
      _JData["update"] = DeviceConfig.UpdateMillis;
      _JData["samplerate"] = DeviceConfig.SampleRate;
      for (int i = 0; i < JCA_EM_CURRENT_SENSES; i++) {
        JsonObject ChannelObj = _JData["Current_" + String (i + 1)].to<JsonObject>();
        ChannelObj["Pin"] = DeviceConfig.CurrentInputs[i].Pin;
        ChannelObj["Offset"] = DeviceConfig.CurrentInputs[i].Offset;
        ChannelObj["Factor"] = DeviceConfig.CurrentInputs[i].Factor;
        ChannelObj["VoltageMap"] = DeviceConfig.CurrentMapping[i];
      }
      for (int i = 0; i < JCA_EM_VOLTAGE_SENSES; i++) {
        JsonObject ChannelObj = _JData["Voltage_" + String (i + 1)].to<JsonObject>();
        ChannelObj["Pin"] = DeviceConfig.VoltageInputs[i].Pin;
        ChannelObj["Offset"] = DeviceConfig.VoltageInputs[i].Offset;
        ChannelObj["Factor"] = DeviceConfig.VoltageInputs[i].Factor;
      }
    }

    void addData(JsonObject _JData) {
      _JData["type"] = "Data";
      for (int i = 0; i < JCA_EM_CURRENT_SENSES; i++) {
        if (PowerInputs[i].Valid) {
          JsonObject ChannelObj = _JData["Channel_" + String (i + 1)].to<JsonObject>();
          JsonObject DataObj = ChannelObj["data"].to<JsonObject>();
          DataObj["VoltageRMS"] = PowerInputs[i].Data.VoltageRMS;
          DataObj["CurrentRMS"] = PowerInputs[i].Data.CurrentRMS;
          DataObj["Power"] = PowerInputs[i].Data.Power;
          DataObj["PowerActiv"] = PowerInputs[i].Data.PowerActiv;
          DataObj["PowerReactiv"] = PowerInputs[i].Data.PowerReactiv;
          DataObj["PowerFactor"] = PowerInputs[i].Data.PowerFactor;

          JsonObject CounterObj = ChannelObj["cnt"].to<JsonObject>();
          CounterObj["Import"] = DeviceCounter.Counters[i].Import;
          CounterObj["Export"] = DeviceCounter.Counters[i].Export;
        }
      }
    }

    void addDetail(JsonObject _JData, int8_t _Channel) {
      if (_Channel == 0) {
        _JData["type"] = "Raw";
        for (int i = 0; i < JCA_EM_CURRENT_SENSES; i++) {
          getRawChannel(_JData, i);
        }
      } else {
        if (_Channel > 0 && _Channel <= JCA_EM_CURRENT_SENSES) {
          _JData["type"] = "Raw";
          int i = _Channel - 1;
          getRawChannel(_JData, i);
        } else {
          _JData["type"] = "Error";
          _JData["message"] = "Channel out of Range";
        }
      }
    }

    void getRawChannel(JsonObject _JData, int8_t _Channel) {
      if (PowerInputs[_Channel].Valid) {
        JsonObject ChannelObj = _JData["Channel_" + String (_Channel + 1)].to<JsonObject>();
        JsonArray ArrVoltage = ChannelObj["voltage"].to<JsonArray>();
        JsonArray ArrCurrent = ChannelObj["current"].to<JsonArray> ();
        for (int x = 0; x < JCA_EM_SAMPLECOUNT; x++) {
          ArrVoltage.add(PowerInputs[_Channel].RawData.Voltage[x]);
          ArrCurrent.add(PowerInputs[_Channel].RawData.Current[x]);
        }
      }
    }

    void addWebSocketCallback (JsonObjectCallback _CB) {
      cbWebSocket = _CB;
    }

    void startZeroTask (JsonObject _JData) {
      InitDone = false;
      OperMode = OperMode_E::ReadZero;
      xTaskCreatePinnedToCore(
        JCA::EM::taskGetZero,
        "ReadData",
        10000,
        NULL,
        1000,
        &HandleReadData,
        1);
    }

    void startCalibrationTask (JsonObject _JData) {
      if (InitDone) {
      CalDone = false;
      OperMode = OperMode_E::ReadCalibration;
      xTaskCreatePinnedToCore(
        JCA::EM::taskCalibrate,
        "ReadData",
        10000,
        NULL,
        1000,
        &HandleReadData,
        1);
      }
    }

    void startReadTask (JsonObject _JData) {
      if (CalDone) {
      OperMode = OperMode_E::ReadValue;
      xTaskCreatePinnedToCore(
        JCA::EM::taskReadData,
        "ReadData",
        10000,
        NULL,
        1000,
        &HandleReadData,
        1);
      }
    }
  }
}
