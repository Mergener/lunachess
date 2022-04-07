#ifndef LUNA_AI_AIMOVEGEN_H
#define LUNA_AI_AIMOVEGEN_H

#include <cstring>
#include <algorithm>

#include "../position.h"
#include "../movegen.h"
#include "../posutils.h"

namespace lunachess::ai {

/**
 * Contains a score for each category of move. Highest scored moves
 * are placed before lowest scored moves and thus, searched before.
 */
struct MoveOrderingScores {
    // Individual scores -- scores claimed
    // by individual moves
    int promotionBaseScore = 5000;
    int promotionPieceTypeScore = 100;
    int captureBaseScore = 3000;
    int castlesScore = 50;
    int placementDeltaMultiplier = 5;
    int mvvLvaPctFactor = 20;
    int killerMoveScore = 1500;

    int mvvLva[PT_COUNT][PT_COUNT] = {
        /*         x-    xP    xN    xB    xR    xQ    xK  */
        /* -- */   0,    0,    0,    0,    0,    0,    0,
        /* Px */   0,    0,   200,  250,  350,  800,  9999,
        /* Nx */   0,  -100,   0,    50,  200,  800,  9999,
        /* Bx */   0,   -60,  -10,   0,   180,  800,  9999,
        /* Rx */   0,  -250, -150, -100,  500,  800,  9999, 
        /* Qx */   0,  -700, -600, -650, -300,  300,  9999,
        /* Kx */   0,    0,    0,    0,    0,    0,   9999
    };

    // Comparative scores -- awarded if a move A has
    // something that B doesn't
    int historyScore = 400;
};

/**
 * Generates and order moves for an alpha-beta pruning based move searcher.
 */
class AIMoveFactory {
public:
    int generateMoves(MoveList& ml, const Position& pos, int currPly, Move pvMove = MOVE_INVALID);
    int generateNoisyMoves(MoveList& ml, const Position& pos, int currPly, Move pvMove = MOVE_INVALID);

    inline void storeKillerMove(Move move, int ply) {
        m_Killers[ply][1] = m_Killers[ply][0];
        m_Killers[ply][0] = move;
    }

    inline void storeHistory(Move move, int depth) {
        m_History[move.getSourcePiece().getColor()][move.getSource()][move.getDest()] += depth*depth;
    }

    inline void resetHistory() {
        std::memset(m_History, 0, sizeof(m_History));
    }

    AIMoveFactory() = default;
    inline AIMoveFactory(const MoveOrderingScores& scores)
        : m_Scores(scores) {
        resetHistory();
    }

    static void initialize();

private:
    MoveOrderingScores m_Scores;

    Move m_Killers[128][2];
    int m_History[CL_COUNT][SQ_COUNT][SQ_COUNT];

    inline bool isKillerMove(Move move, int ply) const {
        return move == m_Killers[ply][0]
            || move == m_Killers[ply][1];
    }

    static inline bool isSquareAttackedByPawn(const Position& pos, Square s, Color pawnColor) {
        return pos.getAttacks(pawnColor, PT_PAWN).contains(s);
    }

    static inline constexpr int getPointValue(PieceType pt) {
        constexpr int PT_VALUE[PT_COUNT] {
                0, 1, 3, 3, 5, 9, 99999
        };

        return PT_VALUE[pt];
    }

    static int getHotmapDelta(Move move);

    enum HasGoodSee : ui8 {
        UNKNOWN,
        YES,
        NO,
    };

    template <bool NOISY>
    int scoreMove(const Position& pos, Move move, int currPly) const {
        int total = 0;

        // Check if killer move
        if (isKillerMove(move, currPly)) {
            total += m_Scores.killerMoveScore;
        }

        if (move.is<MTM_CASTLES>()) {
            // Castling is usually a good option when available
            total += m_Scores.castlesScore;
        }

        // Hotmap delta
        total += getHotmapDelta(move) * m_Scores.placementDeltaMultiplier;

        total -= getPointValue(move.getSourcePiece().getType());

        return total;
    }
};

}

#endif // LUNA_AI_AIMOVEGEN_H
