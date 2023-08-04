#ifndef LUNA_AI_ENDGAME_H
#define LUNA_AI_ENDGAME_H

#include "position.h"

namespace lunachess {

enum EndgameType {
    EG_UNKNOWN = 0,

    //
    // 1 man endgames
    //

    EG_KP_K,
    EG_KR_K,
    EG_KQ_K,

    //
    // 2 men endgames
    //

    EG_KBP_K,
    EG_KBB_K,
    EG_KBN_K,
    EG_KR_KN,
    EG_KR_KB,
    EG_KR_KR,
    EG_KQ_KQ,
};

struct EndgameData {
    EndgameType type = EG_UNKNOWN;

    /**
     * The 'left-hand-side' color of the endgame type.
     * Examples:
     *  In a KP_K endgame, the lhs is the color that controls the pawn.
     *  In a KR_KN endgame, the lhs is the color that controls the rook.
     *
     * If one color of an endgame type has more material than the other, that
     * color will always be the lhs.
     *
     * This field can be ignored when type == EG_UNKNOWN.
     */
    Color lhs = CL_WHITE;
};

namespace endgame {

/**
 * Identifies the type of endgame being played on the specified position, or
 * if the position is not an endgame known by Luna.
 */
EndgameData identify(const Position &pos);

/**
 * Checks whether a king is inside a pawn's "safe square"
 * for promotion, according to the endgame rule of the square.
 * See: https://en.wikibooks.org/wiki/Chess/The_Endgame/Pawn_Endings#The_Rule_of_the_Square
 * @param pawnSquare The promoting pawn's square
 * @param enemyKingSquare The square of the enemy king
 * @param pawnColor The color of the promoting pawn
 * @param colorToMove The current color to move
 * @return True if the enemy king is inside the square and thus, the pawn
 * cannot promote unassisted.
 */
bool isInsideTheSquare(Square pawnSquare, Square enemyKingSquare,
                       Color pawnColor, Color colorToMove);

void initialize();

} // endgame

} // lunachess

#endif // LUNA_AI_ENDGAME_H
