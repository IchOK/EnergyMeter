#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern Preferences DataStorage;
    extern DeviceConfig_T DeviceConfig;
    extern DeviceCounter_T DeviceCounter;
    extern PowerInput_T PowerInputs[JCA_EM_CURRENT_SENSES];

    int16_t getMeanValue(int16_t Samples[], uint8_t Count) {
      int32_t Sum = 0;
      for (int i = 0; i < Count; i++) {
        Sum += Samples[i];
      }
      return Sum /Count;
    }

    void calcData(uint8_t _Channel) {
      uint8_t PinCurrent;
      uint8_t PinVoltage;
      uint16_t OffsetCurrent;
      uint16_t OffsetVoltage;
      float FactorCurrent;
      float FactorVoltage;
      uint8_t MapIndex;

      float U;  // Spannung am Messpunkt
      float I;  // Strom am Messpunkt
      float Usum; // Aufsummierte quadratische Spannung
      float Isum; // Aufsummierter quadratischer Strom
      float Psum; // Aufsummierte Wirkleistung
      
      if (PowerInputs[_Channel].Valid) {
        MapIndex = DeviceConfig.CurrentMapping[_Channel];
        OffsetVoltage = DeviceConfig.VoltageInputs[MapIndex].Offset;
        FactorVoltage = DeviceConfig.VoltageInputs[MapIndex].Factor;
        OffsetCurrent = DeviceConfig.CurrentInputs[_Channel].Offset;
        FactorCurrent = DeviceConfig.CurrentInputs[_Channel].Factor;

        Usum = 0.0;
        Isum = 0.0;
        Psum = 0.0;
        for (int x = 0; x < JCA_EM_SAMPLECOUNT; x++) {
          U = float(PowerInputs[_Channel].RawData.Voltage[x] - OffsetVoltage) * FactorVoltage;
          I = float(PowerInputs[_Channel].RawData.Current[x] - OffsetCurrent) * FactorCurrent;
          Usum += sq(U);
          Isum += sq(I);
          Psum += U * I;
        }

        PowerInputs[_Channel].Data.VoltageRMS = sqrt (Usum / JCA_EM_SAMPLECOUNT);
        PowerInputs[_Channel].Data.CurrentRMS = sqrt (Isum / JCA_EM_SAMPLECOUNT);
        PowerInputs[_Channel].Data.Power = PowerInputs[_Channel].Data.VoltageRMS * PowerInputs[_Channel].Data.CurrentRMS;
        PowerInputs[_Channel].Data.PowerActiv = Psum / JCA_EM_SAMPLECOUNT;
        PowerInputs[_Channel].Data.PowerReactiv = sqrt(sq(PowerInputs[_Channel].Data.Power) - sq(PowerInputs[_Channel].Data.PowerActiv));
        PowerInputs[_Channel].Data.PowerFactor = PowerInputs[_Channel].Data.PowerActiv / PowerInputs[_Channel].Data.Power;

        // Energie zählen
        if (PowerInputs[_Channel].Data.FirstDone) {
          float Factor = float(PowerInputs[_Channel].RawData.StartRead - PowerInputs[_Channel].Data.LastRead) / 3600.0 / 1000.0 / 1000.0;
          if (PowerInputs[_Channel].Data.PowerActiv > 0.0) {
            PowerInputs[_Channel].Data.PartImport += PowerInputs[_Channel].Data.PowerActiv * Factor;
            if (PowerInputs[_Channel].Data.PartImport >= 1.0) {
              uint32_t PartAdd = (uint32_t) PowerInputs[_Channel].Data.PartImport;
              DeviceCounter.Counters[_Channel].Import += PartAdd;
              PowerInputs[_Channel].Data.PartImport -= (float)PartAdd;
            }
          }
          if (PowerInputs[_Channel].Data.PowerActiv < 0.0) {
            PowerInputs[_Channel].Data.PartExport += PowerInputs[_Channel].Data.PowerActiv * Factor;
            if (PowerInputs[_Channel].Data.PartExport >= 1.0) {
              uint32_t PartAdd = (uint32_t) PowerInputs[_Channel].Data.PartExport;
              DeviceCounter.Counters[_Channel].Export += PartAdd;
              PowerInputs[_Channel].Data.PartExport -= (float)PartAdd;
            }
          }
        }
        PowerInputs[_Channel].Data.FirstDone = true;
        PowerInputs[_Channel].Data.LastRead = PowerInputs[_Channel].RawData.StartRead;
      }
    }

  }
}
