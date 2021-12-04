#ifndef LUNACHESS_H
#define LUNACHESS_H

// lunachess::core
#include "core/bitboard.h"
#include "core/bits.h"
#include "core/debug.h"
#include "core/defs.h"
#include "core/move.h"
#include "core/piece.h"
#include "core/position.h"
#include "core/square.h"
#include "core/staticlist.h"
#include "core/strutils.h"
#include "core/types.h"
#include "core/zobrist.h"

// lunachess::ai
#include "ai/aibitboards.h"
#include "ai/basicevaluator.h"
#include "ai/endgame.h"
#include "ai/evaluator.h"
#include "ai/movepicker.h"
#include "ai/openingbook.h"
#include "ai/scorestable.h"
#include "ai/tranpositiontable.h"

namespace lunachess {

void initialize();

}

#endif // LUNACHESS_H