#ifndef LUNACHESS_H
#define LUNACHESS_H

#include "bitboard.h"
#include "bits.h"
#include "chessgame.h"
#include "clock.h"
#include "endgame.h"
#include "lock.h"
#include "move.h"
#include "movegen.h"
#include "perft.h"
#include "piece.h"
#include "position.h"
#include "posutils.h"
#include "staticlist.h"
#include "strutils.h"
#include "types.h"
#include "utils.h"
#include "zobrist.h"

#include "ai/aimovefactory.h"
#include "ai/quiescevaluator.h"
#include "ai/evaluator.h"
#include "ai/search.h"
#include "ai/timemanager.h"
#include "ai/transpositiontable.h"
#include "ai/classiceval/aibitboards.h"
#include "ai/classiceval/classicevaluator.h"
#include "ai/classiceval/hotmap.h"
#include "ai/classiceval/texel.h"
#include "ai/neuraleval/neuraleval.h"
#include "ai/neuraleval/neuralgenetic.h"
#include "ai/neuraleval/nn.h"

namespace lunachess {

void initializeEverything();

} // lunachess

#endif // LUNACHESS_H