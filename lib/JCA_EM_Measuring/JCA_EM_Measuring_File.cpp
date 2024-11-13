#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern InputConfig VoltageInput;
    extern PowerInput CurrentInputs[JCA_EM_MAX_CURRENT];
    extern bool ZeroDone;
    extern bool CalDone;
    extern uint32_t AnalyseUpdate;

    JsonDocument FileMsgJDoc;
    String FileMsgString;

    void fileRead() {
      File CalFile = LittleFS.open (JCA_EM_CALFILE, FILE_READ);
      if (!CalFile) {
        Serial.println("Calibration File not Found");
      } else {
        deserializeJson(FileMsgJDoc, CalFile);
        JsonObject Data = FileMsgJDoc.to<JsonObject>();
        ZeroDone = true;
        CalDone = true;
        // Update
        AnalyseUpdate = Data["Voltage"]["Update"].as<uint32_t> ();

        // Spannungswerte
        VoltageInput.Factor = Data["Voltage"]["Factor"].as<float> ();
        if (VoltageInput.Factor == 0.0) {
          CalDone = false;
        }
        VoltageInput.Offset = Data["Voltage"]["Offset"].as<int> ();
        if (VoltageInput.Offset == 0) {
          ZeroDone = false;
        }

        // Stromwerte
        for (int i=0; i<JCA_EM_MAX_CURRENT; i++) {
          if (CurrentInputs[i].Config.Pin > 0) {
            String Name = "Current" + String(i);
            CurrentInputs[i].Config.Factor = Data[Name]["Factor"].as<float> ();
            if (CurrentInputs[i].Config.Factor == 0.0) {
              CalDone = false;
            }
            CurrentInputs[i].Config.Offset = Data[Name]["Offset"].as<int> ();
            if (CurrentInputs[i].Config.Offset == 0) {
              ZeroDone = false;
            }
            CurrentInputs[i].Counter.Import = Data[Name]["Import"].as<uint32_t> ();
            CurrentInputs[i].Counter.Export = Data[Name]["Export"].as<uint32_t> ();
          }
        }

        CalFile.close();
      }

    }

    void fileSave() {
      File CalFile = LittleFS.open (JCA_EM_CALFILE, FILE_READ);
      if (!CalFile) {
        Serial.println("Calibration File cant write");
      } else {
        FileMsgJDoc.clear();
        JsonObject Data = FileMsgJDoc.to<JsonObject>();
        // Update
        Data["Voltage"]["Update"] = AnalyseUpdate;

        // Spannungswerte
        Data["Voltage"]["Factor"] = VoltageInput.Factor;
        Data["Voltage"]["Offset"] = VoltageInput.Offset;

        // Stromwerte
        for (int i=0; i<JCA_EM_MAX_CURRENT; i++) {
          String Name = "Current" + String(i);
          Data[Name]["Factor"] = CurrentInputs[i].Config.Factor;
          Data[Name]["Offset"] = CurrentInputs[i].Config.Offset;
          Data[Name]["Import"] = CurrentInputs[i].Counter.Import;
          Data[Name]["Export"] = CurrentInputs[i].Counter.Export;
        }

        serializeJson(Data, CalFile);
        CalFile.close();
      }
    }
  }
}