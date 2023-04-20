#ifndef LUNA_AI_AIMOVEGEN_H
#define LUNA_AI_AIMOVEGEN_H

#include <cstring>
#include <algorithm>

#include "../position.h"
#include "../movegen.h"
#include "../staticanalysis.h"

namespace lunachess::ai {

/**
 * Contains a score for each category of move. Highest scored moves
 * are placed before lowest scored moves and thus, searched before.
 */
struct MoveOrderingScores {
    // Individual scores -- scores claimed
    // by individual moves
    int placementDeltaMultiplier = 4;
    int guardValueMultiplier = 6;

    int squareGuardedByPawnPenalty = 50;

    int mvvLva[PT_COUNT][PT_COUNT] = {
        /*         x-    xP    xN    xB    xR    xQ    xK  */
        /* -- */ { 0,    0,    0,    0,    0,    0,    0   },
        /* Px */ { 0,   105,  205,  305,  405,  505,  9999 },
        /* Nx */ { 0,   104,  204,  304,  404,  504,  9999 },
        /* Bx */ { 0,   103,  203,  303,  403,  503,  9999 },
        /* Rx */ { 0,   102,  202,  302,  600,  502,  9999 },
        /* Qx */ { 0,   101,  201,  301,  401,  501,  9999 },
        /* Kx */ { 0,   100,  200,  300,  400,  500,  9999 },
    };
};

/**
 * Generates and order moves for an alpha-beta pruning based move searcher.
 */
class AIMoveFactory {
public:
    int generateMoves(MoveList& ml, const Position& pos, int currPly, Move pvMove = MOVE_INVALID);
    int generateNoisyMoves(MoveList& ml, const Position& pos, int currPly, Move pvMove = MOVE_INVALID) const;

    inline void storeKillerMove(Move move, int ply) {
        m_Killers[ply][1] = m_Killers[ply][0];
        m_Killers[ply][0] = move;
    }

    inline bool isKillerMove(Move move, int ply) const {
        return move == m_Killers[ply][0]
               || move == m_Killers[ply][1];
    }

    inline void storeHistory(Move move, int depth) {
        m_History[move.getSourcePiece().getColor()][move.getSource()][move.getDest()] += depth*depth;
    }

    inline void resetHistory() {
        std::memset(m_History, 0, sizeof(m_History));
    }

    inline AIMoveFactory() {
        resetHistory();
    }
    inline AIMoveFactory(const MoveOrderingScores& scores)
        : m_Scores(scores) {
        resetHistory();
    }

    static void initialize();

private:
    MoveOrderingScores m_Scores;

    Move m_Killers[128][2];
    int m_History[CL_COUNT][SQ_COUNT][SQ_COUNT];

    inline int getMoveHistory(Move move) {
        return m_History[move.getSourcePiece().getColor()][move.getSource()][move.getDest()];
    }

    int scoreQuietMove(const Position& pos, Move move) const;

    static inline constexpr int getPointValue(PieceType pt) {
        return getPiecePointValue(pt);
    }

    static int getHotmapDelta(Move move);
};

}

#endif // LUNA_AI_AIMOVEGEN_H
