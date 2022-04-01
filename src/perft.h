#ifndef LUNA_PEFT_H
#define LUNA_PERFT_H

#include "position.h"

namespace lunachess {

ui64 perft(const Position& pos, int depth, bool pseudoLegal = false);

} // lunachess

#endif // LUNA_PERFT_H