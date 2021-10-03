#include "lunachess.h"

namespace lunachess {

static bool s_Initialized = false;

void initialize() {
	if (s_Initialized) {
		return;
	}
	s_Initialized = true;

	Piece::initialize();
	zobrist::initialize();
	squares::initialize();
	bitboards::initialize();

	lunachess::ai::aibitboards::initialize();
	lunachess::ai::endgame::initialize();
}

}