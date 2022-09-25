#ifndef LUNACHESS_H
#define LUNACHESS_H

#include "bitboard.h"
#include "bits.h"
#include "chessgame.h"
#include "clock.h"
#include "endgame.h"
#include "move.h"
#include "movegen.h"
#include "perft.h"
#include "piece.h"
#include "position.h"
#include "posutils.h"
#include "staticlist.h"
#include "strutils.h"
#include "types.h"
#include "uci.h"
#include "utils.h"
#include "zobrist.h"

namespace lunachess {

void initializeEverything();

}

#endif // LUNACHESS_H