#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    Preferences DataStorage;
    DeviceConfig_T DeviceConfig;
    DeviceCounter_T DeviceCounter;
    PowerInput_T PowerInputs[JCA_EM_CURRENT_SENSES];
    OperMode_E OperMode = OperMode_E::Idle;
    bool InitDone = false;
    bool CalDone = false;
    float Cal_VoltageRMS;
    float Cal_CurrentRMS;
    unsigned long LastUpdate;
    JsonObjectCallback cbWebSocket;
  }
}
