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
    extern uint32_t LastUpdate;
    extern JsonObjectCallback cbWebSocket;

    void taskGetZero (void *_TaskParameters) {
      uint32_t SampleCount = JCA_EM_CAL_TIME / JCA_EM_CAL_RATE;
      uint64_t SumValue;
      uint16_t Offset;
      int16_t MeanSamples[JCA_EM_MEAN_SAMPLES];
      uint8_t Pin;
      uint8_t Index;
      uint16_t Counter;
      unsigned long ActMicros;
      unsigned long LastRead;
      bool LastPlus;
      bool ActPlus;

      InitDone = false;
      CalDone = false;
      //----------------------------
      // Spannungseingänge Nullen
      for (int i = 0; i < JCA_EM_VOLTAGE_SENSES; i++) {
        Pin = DeviceConfig.VoltageInputs[i].Pin;
        if (Pin > 0) {
          // Messwerte aufnehmen und Summe bilden
          SumValue = 0;
          Counter = 0;
          ActMicros = micros ();
          LastRead = ActMicros - JCA_EM_CAL_RATE;
          while (Counter < SampleCount) {
            if (ActMicros - LastRead >= JCA_EM_CAL_RATE) {
              SumValue += (uint64_t)analogRead (Pin);
              Counter++;
              LastRead = ActMicros;
            }
            ActMicros = micros ();
          }

          // Mittelwert berechnen und als Nullpunkt speichern
          DeviceConfig.VoltageInputs[i].Offset = (uint16_t)(SumValue / (uint64_t)SampleCount);

          Serial.print ("Zero Voltage PIN:");
          Serial.println (Pin);

          // Kurz werten um anderen Tasks die Möglichkeit zur ausführung zu geben
          vTaskDelay (200U);
        }
      }

      //----------------------------
      // Stromeingänge Nullen
      for (int i = 0; i < JCA_EM_CURRENT_SENSES; i++) {
        Pin = DeviceConfig.CurrentInputs[i].Pin;
        if (Pin > 0) {
          // Messwerte aufnehmen und Summe bilden
          SumValue = 0;
          Counter = 0;
          ActMicros = micros ();
          LastRead = ActMicros - JCA_EM_CAL_RATE;
          while (Counter < SampleCount) {
            if (ActMicros - LastRead >= JCA_EM_CAL_RATE) {
              SumValue += (uint64_t)analogRead (Pin);
              Counter++;
              LastRead = ActMicros;
            }
            ActMicros = micros ();
          }

          // Mittelwert berechnen und als Nullpunkt speichern
          DeviceConfig.CurrentInputs[i].Offset = (uint16_t)(SumValue / (uint64_t)SampleCount);

          Serial.print ("Zero Current PIN:");
          Serial.println (Pin);

          // Kurz werten um anderen Tasks die Möglichkeit zur ausführung zu geben
          vTaskDelay (200U);
        }
      }

      //----------------------------
      // Ferquenz ermitteln
      if (DeviceConfig.PeriodMicros == 0) {
        Serial.println("Start Frequency");
        Pin = 0;
        for (int i = 0; i < JCA_EM_VOLTAGE_SENSES; i++) {
          if (DeviceConfig.VoltageInputs[i].Pin > 0) {
            Pin = DeviceConfig.VoltageInputs[i].Pin;
            Offset = DeviceConfig.VoltageInputs[i].Offset;
            break;
          }
        }

        if (Pin > 0) {
          // Analyse Array füllen
          for (int i = 0; i < JCA_EM_MEAN_SAMPLES; i++) {
              MeanSamples[i] = analogRead (Pin) - Offset;
          }
          ActPlus = getMeanValue(MeanSamples, JCA_EM_MEAN_SAMPLES) > 0;
          LastPlus = ActPlus;
          Counter = 0;
          Index = 0;
          
          // Nulldruchgänge ermitteln
          while (Counter <= JCA_EM_FREQ_PERIODES) {
            MeanSamples[Index] = analogRead (Pin) - Offset;
            ActPlus = getMeanValue (MeanSamples, JCA_EM_MEAN_SAMPLES) > 0;
            if (!LastPlus && ActPlus) {
              // Nulldurchgang erkannt
              if (Counter == 0) {
                ActMicros = micros();
              }
              Counter++;
            }
            LastPlus = ActPlus;
            Index++;
            if (Index >= JCA_EM_MEAN_SAMPLES) {
              Index = 0;
            }
          }

          // Periodendauer berechnen
          DeviceConfig.PeriodMicros = (micros() - ActMicros) / JCA_EM_FREQ_PERIODES;
          DeviceConfig.SampleRate = DeviceConfig.PeriodMicros / JCA_EM_SAMPLES_PERPEROIDE;
        }
        Serial.println("Done Frequency");
      }

      InitDone = true;
      configSave();
      vTaskDelete(NULL);
    }

    void taskCalibrate (void *_TaskParameters) {
      uint32_t SampleCount = JCA_EM_CAL_PERIODES * JCA_EM_SAMPLES_PERPEROIDE;
      int64_t SumValue;
      int64_t ActValue;
      uint16_t Offset;
      uint8_t Pin;
      uint8_t Index;
      uint16_t Counter;
      unsigned long ActMicros;
      unsigned long LastRead;
      uint16_t Samples[JCA_EM_CAL_PERIODES * JCA_EM_SAMPLES_PERPEROIDE];

      CalDone = false;
      //----------------------------
      // Spannungseingänge kallibrieren
      for (int i = 0; i < JCA_EM_VOLTAGE_SENSES; i++) {
        Pin = DeviceConfig.VoltageInputs[i].Pin;
        if (Pin > 0) {
          // Messwerte aufnehmen und Quadradsumme bilden
          Offset = DeviceConfig.VoltageInputs[i].Offset;
          SumValue = 0;
          Counter = 0;
          ActMicros = micros ();
          LastRead = ActMicros - DeviceConfig.SampleRate;
          while (Counter < SampleCount) {
            if (ActMicros - LastRead >= DeviceConfig.SampleRate) {
              Samples[Counter] = analogRead (Pin);
              Counter++;
              LastRead = ActMicros;
            }
            ActMicros = micros ();
          }

          // RMS-Wert berechnen und Kallibrierungsfaktor speichern speichern
          SumValue = getSqSumMean(Samples, SampleCount, JCA_EM_CAL_PERIODES, Offset);
          DeviceConfig.VoltageInputs[i].Factor = Cal_VoltageRMS / sqrt((float)SumValue / (float)JCA_EM_SAMPLES_PERPEROIDE);

          // Kurz werten um anderen Tasks die Möglichkeit zur ausführung zu geben
          vTaskDelay (200U);
        }
      }

      //----------------------------
      // Stromeingänge kallibrieren
      for (int i = 0; i < JCA_EM_CURRENT_SENSES; i++) {
        Pin = DeviceConfig.CurrentInputs[i].Pin;
        if (Pin > 0) {
          // Messwerte aufnehmen und Quadradsumme bilden
          Offset = DeviceConfig.CurrentInputs[i].Offset;
          SumValue = 0;
          Counter = 0;
          ActMicros = micros ();
          LastRead = ActMicros - DeviceConfig.SampleRate;
          while (Counter < SampleCount) {
            if (ActMicros - LastRead >= DeviceConfig.SampleRate) {
              Samples[Counter] = analogRead (Pin);
              Counter++;
              LastRead = ActMicros;
            }
            ActMicros = micros ();
          }

          // RMS-Wert berechnen und Kallibrierungsfaktor speichern speichern
          SumValue = getSqSumMean(Samples, SampleCount, JCA_EM_CAL_PERIODES, Offset);
          DeviceConfig.CurrentInputs[i].Factor = Cal_CurrentRMS / sqrt((float)SumValue / (float)JCA_EM_SAMPLES_PERPEROIDE);

          // Kurz werten um anderen Tasks die Möglichkeit zur ausführung zu geben
          vTaskDelay (200U);
        }
      }

      CalDone = true;
      configSave ();
      vTaskDelete (NULL);
    }

    void taskReadData (void *_TaskParameters) {
      uint8_t PinCurrent;
      uint8_t PinVoltage;
      uint16_t OffsetCurrent;
      uint16_t OffsetVoltage;
      float FactorCurrent;
      float FactorVoltage;
      uint8_t MapIndex;
      uint16_t Counter;
      unsigned long ActMicros;
      unsigned long LastRead;

      while (true) {
        uint16_t Current[JCA_EM_SAMPLECOUNT];
        uint16_t Voltage[JCA_EM_SAMPLECOUNT];

        //----------------------------
        // Strom/Spannungswerte einlesen
        for (int i = 0; i < JCA_EM_CURRENT_SENSES; i++) {
          // Messpunkt Konfig prüfen
          MapIndex = DeviceConfig.CurrentMapping[i];
          if (MapIndex >= 0 && MapIndex < JCA_EM_VOLTAGE_SENSES) {
            PinVoltage = DeviceConfig.VoltageInputs[MapIndex].Pin;
          } else {
            PinVoltage = 0;
          }
          PinCurrent = DeviceConfig.CurrentInputs[i].Pin;
          PowerInputs[i].Valid = PinVoltage > 0 && PinCurrent > 0;
          if (PowerInputs[i].Valid) {
            // Messwerte aufnehmen und Quadradsumme bilden
            Counter = 0;
            ActMicros = micros ();
            LastRead = ActMicros - DeviceConfig.SampleRate;
            PowerInputs[i].RawData.StartRead = ActMicros;
            while (Counter < JCA_EM_SAMPLECOUNT) {
              if (ActMicros - LastRead >= DeviceConfig.SampleRate) {
                Current[Counter] = analogRead(PinCurrent);
                Voltage[Counter] = analogRead(PinVoltage);
                Counter++;
                LastRead = ActMicros;
              }
              ActMicros = micros ();
            }
          }

          // Daten in Speicher umkopieren und Werte berechnen
          std::memcpy(PowerInputs[i].RawData.Current, Current, sizeof(Current));
          std::memcpy(PowerInputs[i].RawData.Voltage, Voltage, sizeof(Voltage));
          calcData(i);

          // Kurz werten um anderen Tasks die Möglichkeit zur ausführung zu geben
          vTaskDelay (200U);
        }
      }
    }

    void loop () {
      bool DoUpdate = false;
      JsonDocument JDoc;
      JsonObject JData = JDoc["data"].to<JsonObject>();
      unsigned long ActMillis = millis();

      // Prüfen ob Kalibrierung erfolgt ist
      switch (OperMode) {
        case OperMode_E::Idle:
          // gespeicherte Daten einlasen
          dataRead();

          if (!InitDone) {
            OperMode = OperMode_E::WaitZero;
          } else if (!CalDone) {
            OperMode = OperMode_E::WaitCalibration;
          } else {
            OperMode = OperMode_E::ReadValue;
            JData["mode"] = OperMode;
            startReadTask (JData);
            cbWebSocket(JData);
          }
          break;

        case OperMode_E::WaitZero :
          // Warten auf Nullen befehlt, der Task wird über ein Kommando gestartet
          // - Speisung über USB und Stromumformer nicht verbunden.
          DoUpdate = true;
          break;

        case OperMode_E::ReadZero :
          // Nullpunkt ermitteln Warten bis initalisierung abgeschlossen ist
          if (InitDone) {
            OperMode = OperMode_E::WaitCalibration;
          } else {
            DoUpdate = true;
          }

          break;

        case OperMode_E::WaitCalibration:
          // Warten auf Kalibrierung befehlt, der Task wird über ein Kommando gestartet
          // - Netzspannung muss anliegen und es muss ein Definierte Strom fliessen.
          DoUpdate = true;
          break;

        case OperMode_E::ReadCalibration:
          // Nullpunkt ermitteln Warten bis initalisierung abgeschlossen ist
          if (CalDone) {
            OperMode = OperMode_E::ReadValue;
            startReadTask(JData);
            cbWebSocket(JData);
          } else {
            DoUpdate = true;
          }

          break;

        case OperMode_E::ReadValue :
          // Messdaten senden
          DoUpdate = true;
          break;

        default:
          OperMode = OperMode_E::Idle;

          break;
      }

      // Update senden
      if (ActMillis - LastUpdate >= DeviceConfig.UpdateMillis && DoUpdate) {
        JData["mode"] = OperMode;
        addData(JData);
        cbWebSocket(JData);
        LastUpdate = ActMillis;
      }
    }
  }
}
