#ifndef LUNA_PERFT_H
#define LUNA_PERFT_H

#include "position.h"

namespace lunachess {

ui64 perft(const Position& pos, int depth, bool log = true, bool pseudoLegal = false, bool algNotation = false);

} // lunachess

#endif // LUNA_PERFT_H