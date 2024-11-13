#include <JCA_EM_Measuring.h>

namespace JCA {
  namespace EM {
    InputConfig VoltageInput;
    PowerInput CurrentInputs[JCA_EM_MAX_CURRENT];
    int ActInput = 0;
    OperMode_E OperMode = OperMode_E::Idle;
    bool ZeroDone = false;
    bool CalDone = false;
    float Cal_VoltageRMS;
    float Cal_CurrentRMS;
    uint32_t LastUpdate;
    uint32_t AnalyseUpdate = 1000;
    JsonObjectCallback cbWebSocket;
  }
}
