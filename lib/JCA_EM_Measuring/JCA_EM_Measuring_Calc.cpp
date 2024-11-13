#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern InputConfig VoltageInput;
    extern PowerInput CurrentInputs[JCA_EM_MAX_CURRENT];
    extern bool ZeroDone;
    extern bool CalDone;
    extern float Cal_VoltageRMS;
    extern float Cal_CurrentRMS;

    void calcSaveZero (JsonObject _JData) {
      for (int i = 0; i < JCA_EM_MAX_CURRENT; i++) {
        if (CurrentInputs[i].Config.Pin > 0) {
          uint32_t RawU = 0;
          uint32_t RawI = 0;
          for (int x = 0; x < CurrentInputs[i].RawData.Counter - 1; x++) {
            RawU += CurrentInputs[i].RawData.Voltage[x];
            RawI += CurrentInputs[i].RawData.Current[x];
          }
          VoltageInput.Offset = RawU / CurrentInputs[i].RawData.Counter;
          CurrentInputs[i].Config.Offset = RawI / CurrentInputs[i].RawData.Counter;

          JsonObject ChannelObj = _JData["Channel_" + String (i + 1)].to<JsonObject>();
          ChannelObj["VoltageOffset"] = VoltageInput.Offset;
          ChannelObj["CurrentOffset"] = CurrentInputs[i].Config.Offset;
        }
      }
      ZeroDone = true;
      // Save Date to File
      fileSave();
    }

    void calcSaveCalibration (JsonObject _JData) {
      for (int i=0; i<JCA_EM_MAX_CURRENT; i++) {
        if (CurrentInputs[i].RawData.Done && CurrentInputs[i].Config.Pin > 0) {
          // Faktor für Spannungsmessung errechnen
          float Usum = 0.0;
          float Isum = 0.0;
          float DeltaT;
          int LastIndex = CurrentInputs[i].RawData.Counter - 1;
          for (int x = 0; x < LastIndex; x++) {
            DeltaT = float(CurrentInputs[i].RawData.TimeMicros[x+1] - CurrentInputs[i].RawData.TimeMicros[x]);
            Usum += sq(float((CurrentInputs[i].RawData.Voltage[x] + CurrentInputs[i].RawData.Voltage[x+1]) / 2 - VoltageInput.Offset)) * DeltaT;
            Isum += sq(float((CurrentInputs[i].RawData.Current[x] + CurrentInputs[i].RawData.Current[x+1]) / 2 - CurrentInputs[i].Config.Offset)) * DeltaT;
          }

          DeltaT = float(CurrentInputs[i].RawData.TimeMicros[LastIndex] - CurrentInputs[i].RawData.Voltage[0]);
          VoltageInput.Factor = Cal_VoltageRMS / sqrt (Usum / DeltaT);
          CurrentInputs[i].Config.Factor = Cal_CurrentRMS / sqrt(Isum / DeltaT);

          JsonObject ChannelObj = _JData["Channel_" + String (i + 1)].to<JsonObject>();
          ChannelObj["VoltageFactor"] = VoltageInput.Factor;
          ChannelObj["CurrentFactor"] = CurrentInputs[i].Config.Factor;
        }
      }
      CalDone = true;
      // Save Data to File
      fileSave();
    }

    void calcData() {
      float U;  // Spannung am Messpunkt
      float I;  // Strom am Messpunkt
      float Usum; // Aufsummierte quadratische Spannung
      float Isum; // Aufsummierter quadratischer Strom
      float Psum; // Aufsummierte Wirkleistung
      for (int i = 0; i < JCA_EM_MAX_CURRENT; i ++) {
        if (CurrentInputs[i].RawData.Done) {
          Usum = 0.0;
          Isum = 0.0;
          Psum = 0.0;
          float DeltaT;
          int LastIndex = CurrentInputs[i].RawData.Counter - 1;
          for (int x = 0; x < LastIndex; x++) {
            DeltaT = float(CurrentInputs[i].RawData.TimeMicros[x+1] - CurrentInputs[i].RawData.TimeMicros[x]);
            U = float((CurrentInputs[i].RawData.Voltage[x] + CurrentInputs[i].RawData.Voltage[x+1]) / 2 - VoltageInput.Offset) * VoltageInput.Factor;
            I = float((CurrentInputs[i].RawData.Current[x] + CurrentInputs[i].RawData.Current[x+1]) / 2 - CurrentInputs[i].Config.Offset) * CurrentInputs[i].Config.Factor;
            Usum += sq(U * DeltaT);
            Isum += sq(I * DeltaT);
            Psum += U * I * DeltaT;
          }

          DeltaT = float(CurrentInputs[i].RawData.TimeMicros[LastIndex] - CurrentInputs[i].RawData.Voltage[0]);
          CurrentInputs[i].Data.VoltageRMS = sqrt(Usum / DeltaT);
          CurrentInputs[i].Data.CurrentRMS = sqrt(Isum / DeltaT);
          CurrentInputs[i].Data.Power = CurrentInputs[i].Data.VoltageRMS * CurrentInputs[i].Data.CurrentRMS;
          CurrentInputs[i].Data.PowerActiv = Psum / DeltaT;
          CurrentInputs[i].Data.PowerReactiv = sqrt(sq(CurrentInputs[i].Data.Power) - sq(CurrentInputs[i].Data.PowerActiv));
          CurrentInputs[i].Data.PowerFactor = CurrentInputs[i].Data.PowerActiv / CurrentInputs[i].Data.Power;
          CurrentInputs[i].Data.Done = true;

          // Energie zählen
          if (CurrentInputs[i].RawData.FirstDone) {
            float Factor = float(CurrentInputs[i].RawData.StartRead - CurrentInputs[i].RawData.LastRead) / 3600.0 / 1000.0 / 1000.0;
            if (CurrentInputs[i].Data.PowerActiv > 0.0) {
              CurrentInputs[i].Counter.PartImport += CurrentInputs[i].Data.PowerActiv * Factor;
              if (CurrentInputs[i].Counter.PartImport >= 1.0) {
                uint32_t PartAdd = (uint32_t) CurrentInputs[i].Counter.PartImport;
                CurrentInputs[i].Counter.Import += PartAdd;
                CurrentInputs[i].Counter.PartImport -= (float)PartAdd;
              }
            }
            if (CurrentInputs[i].Data.PowerActiv < 0.0) {
              CurrentInputs[i].Counter.PartExport += CurrentInputs[i].Data.PowerActiv * Factor;
              if (CurrentInputs[i].Counter.PartExport >= 1.0) {
                uint32_t PartAdd = (uint32_t) CurrentInputs[i].Counter.PartExport;
                CurrentInputs[i].Counter.Export += PartAdd;
                CurrentInputs[i].Counter.PartExport -= (float)PartAdd;
              }
            }
          }
          CurrentInputs[i].RawData.FirstDone = true;
          CurrentInputs[i].RawData.LastRead = CurrentInputs[i].RawData.StartRead;

          // Messung wieder freigeben
          CurrentInputs[i].RawData.Done = false;
        }
      }
    }

  }
}
