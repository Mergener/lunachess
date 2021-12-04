#ifndef LUNA_ENDGAME_H
#define LUNA_ENDGAME_H

#include "../core/square.h"

namespace lunachess::ai::endgame {

enum class EndgameType {
	K_KQ,
	K_KR
};

void initialize();

}

#endif // LUNA_ENDGAME_H
