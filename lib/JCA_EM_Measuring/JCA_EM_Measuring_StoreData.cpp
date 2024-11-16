#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern Preferences DataStorage;
    extern DeviceConfig_T DeviceConfig;
    extern DeviceCounter_T DeviceCounter;
    extern bool InitDone;
    extern bool CalDone;

    void dataRead() {
      DataStorage.begin (JCA_EM_STORAGENAME, true);
      if (DataStorage.isKey(JCA_EM_TAG_CONFIG)) {
        DataStorage.getBytes(JCA_EM_TAG_CONFIG, &DeviceConfig, sizeof(DeviceConfig));
        // Prüfen ob die Initialisierung / Kalibrierung bereits erfolgt ist
        InitDone = true;
        CalDone = true;
        if (DeviceConfig.PeriodMicros > 0) {
          for (int i=0; i<JCA_EM_CURRENT_SENSES; i++) {
            if (DeviceConfig.CurrentInputs[i].Pin) {
              if (DeviceConfig.CurrentInputs[i].Offset == 0) {
                InitDone = false;
              }
              if (DeviceConfig.CurrentInputs[i].Factor == 0.0) {
                CalDone = false;
              }
            }
          }
          for (int i=0; i<JCA_EM_VOLTAGE_SENSES; i++) {
            if (DeviceConfig.VoltageInputs[i].Pin) {
              if (DeviceConfig.VoltageInputs[i].Offset == 0) {
                InitDone = false;
              }
              if (DeviceConfig.VoltageInputs[i].Factor == 0.0) {
                CalDone = false;
              }
            }
          }
        } else {
          InitDone = false;
        }
        if (!InitDone) {
          CalDone = false;
        }
        if (DataStorage.isKey(JCA_EM_TAG_COUNTER)) {
          DataStorage.getBytes(JCA_EM_TAG_COUNTER, &DeviceCounter, sizeof(DeviceCounter));
        } else {
          Serial.println ("No Counter data saved");
        }
      } else {
        Serial.println("No Calibration data saved");
      }
      DataStorage.end();

      if (DeviceConfig.UpdateMillis == 0) {
        DeviceConfig.UpdateMillis = 1000;
      }
    }

    void configSave() {
      Serial.println ("Save Config");
      DataStorage.begin (JCA_EM_STORAGENAME, false);
      DataStorage.putBytes(JCA_EM_TAG_CONFIG, &DeviceConfig, sizeof(DeviceConfig));
      DataStorage.end ();
    }

    void counterSave() {
      Serial.println ("Save Counter");
      DataStorage.begin (JCA_EM_STORAGENAME, false);
      DataStorage.putBytes(JCA_EM_TAG_COUNTER, &DeviceCounter, sizeof(DeviceCounter));
      DataStorage.end ();
    }
  }
}