#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    extern InputConfig VoltageInput;
    extern PowerInput CurrentInputs[JCA_EM_MAX_CURRENT];
    extern int ActInput;
    extern OperMode_E OperMode;
    extern bool CalDone;
    extern uint32_t AnalyseUpdate;
    extern JsonObjectCallback cbWebSocket;

    void taskReadData (void *_TaskParameters) {
      const TickType_t DelayTicks = 100U / portTICK_PERIOD_MS;
      // TaskLoop
      while (true) {
        // Prüfen ob der Eingang bereit ist
        if (CurrentInputs[ActInput].Config.Pin != 0 && !CurrentInputs[ActInput].RawData.Done) {
          uint16_t VoltagePin = VoltageInput.Pin;
          uint16_t CurrentPin = CurrentInputs[ActInput].Config.Pin;
          int VoltageOffset = VoltageInput.Offset;
          int CurrentOffset = CurrentInputs[ActInput].Config.Offset;
          uint16_t Counter = 0;
          uint32_t SampleRate = JCA_EM_PERIODE_US / JCA_EM_SAMPLES;
          uint32_t StartRead = micros ();
          uint32_t LastRead = StartRead - SampleRate;
          uint32_t ActMicros = StartRead;
          while (Counter <= JCA_EM_SAMPLEPERIODES * JCA_EM_SAMPLES) {
            if (ActMicros - LastRead >= SampleRate) {
              CurrentInputs[ActInput].RawData.Voltage[Counter] = analogRead (VoltagePin);
              CurrentInputs[ActInput].RawData.Current[Counter] = analogRead (CurrentPin);
              CurrentInputs[ActInput].RawData.TimeMicros[Counter] = ActMicros - StartRead;
              Counter++;
              LastRead = ActMicros;
            }
            ActMicros = micros ();
          }
          CurrentInputs[ActInput].RawData.StartRead = StartRead;
          CurrentInputs[ActInput].RawData.Counter = Counter;
          CurrentInputs[ActInput].RawData.Done = true;
          vTaskDelay (DelayTicks);
        } else {
          vTaskDelay (DelayTicks / 10);
        }

        // Nächsten Messpunkt anwählen
        ActInput++;
        if (ActInput >= JCA_EM_MAX_CURRENT || ActInput < 0) {
          ActInput = 0;
        }
      }
    }

    void taskAnalysData (void *_TaskParameters) {
      const TickType_t DelayTicks = AnalyseUpdate / portTICK_PERIOD_MS;
      bool StepDone;
      bool AnyDone;
      JsonDocument JDoc;
      while (true) {
        JDoc.clear ();
        JsonObject Data = JDoc.to<JsonObject> ();
        // Prüfen ob Kalibrierung erfolgt ist
        Data["mode"] = OperMode;
        switch (OperMode) {
          case OperMode_E::Idle:
            for (int i = 0; i < JCA_EM_MAX_CURRENT; i++) {
              CurrentInputs[i].RawData.Done = false;
            }
            fileRead();
            if (CalDone) {
              OperMode = OperMode_E::ReadValue;
            }

            break;

          case OperMode_E::ReadZero :
            // Nullpunkt ermitteln, Speisung über USB und Stromumformer nicht verbunden.
            StepDone = true;
            // Prüfen ob alle EIngänge die Messung abgeschlossen haben
            for (int i = 0; i < JCA_EM_MAX_CURRENT; i++) {
              if (CurrentInputs[i].Config.Pin > 0) {
                if (!CurrentInputs[i].RawData.Done) {
                  StepDone = false;
                }
              }
            }

            if (StepDone) {
              OperMode = OperMode_E::DoneZero;
            }

            break;

          case OperMode_E::DoneZero :
            // Warten dass Nullpunkte gespeichert werden

            break;

          case OperMode_E::ReadCalibration :
            // Messwerte für Kallibrierung aufzeichnen, Netzspannung muss anliegen und es muss ein Definierte Strom fliessen
            StepDone = true;
            // Prüfen ob alle EIngänge die Messung abgeschlossen haben
            for (int i = 0; i < JCA_EM_MAX_CURRENT; i++) {
              if (CurrentInputs[i].Config.Pin > 0) {
                if (!CurrentInputs[i].RawData.Done) {
                  StepDone = false;
                }
              }
            }

            if (StepDone){
              OperMode = OperMode_E::DoneCalibration;
            }

            break;

          case OperMode_E::DoneCalibration :
            // Warten dass Kalibrierdaten gespeichert werden
            break;

          case OperMode_E::ReadValue :
            // Zyklisch die Werte der abgeschlossenen Messungen berechen0
            calcData ();
            Data["type"] = "Value";
            addData(Data);
            break;

          case OperMode_E::ReadRaw :
            // Nur Rohwerte lesen, keine Nachbearbeitung
            Data["type"] = "Raw";
            addDetail(Data);
            break;

          default:
            OperMode = OperMode_E::Idle;

            break;
        }
        // Callback Data
        cbWebSocket (Data);
        // Tast Sleep
        vTaskDelay (DelayTicks);
      }
    }
  }
}
