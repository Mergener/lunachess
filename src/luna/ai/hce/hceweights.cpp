#include "hceweights.h"

#include <incbin/incbin.h>

namespace lunachess::ai {

INCTXT(_Weights, HCE_WEIGHTS_FILE);

static HCEWeightTable s_DefaultTable;

const HCEWeightTable* getDefaultHCEWeights() {
    return &s_DefaultTable;
}

void initializeDefaultHCEWeights() {
    s_DefaultTable = nlohmann::json::parse(g_WeightsData);
}

}