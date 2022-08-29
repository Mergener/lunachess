#include "neuralgenetic.h"

#include <nlohmann/json.hpp>

namespace lunachess::ai::neural {

void Agent::mutate(int mutationRatePct) {
    m_Eval.getNetwork().mutate(mutationRatePct);
}

nlohmann::json Agent::toJson() const {

}

void Agent::fromJson(const nlohmann::json& json) {

}


} // lunachess::ai::neural