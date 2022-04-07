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
    int generateMoves(MoveList& ml, Position& pos, int currPly, Move pvMove = MOVE_INVALID);
    int generateNoisyMoves(MoveList& ml, Position& pos, int currPly, Move pvMove = MOVE_INVALID);

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

    struct SortContext {
        int currPly = 0;
        Move pvMove;
        HasGoodSee hasGoodSee[SQ_COUNT][SQ_COUNT];

        inline SortContext() {
            std::memset(hasGoodSee, 0, sizeof(hasGoodSee));
        }
    };

    template <bool NOISY>
    bool compareMoves(Position& pos, Move a, Move b, SortContext& ctx) const {
        int aScore = 0;
        int bScore = 0;

        // PV moves (hash moves) always come first
        if (a == ctx.pvMove) {
            return true;
        }
        if (b == ctx.pvMove) {
            return false;
        }

        // Then come good captures
        if constexpr (NOISY) {
            // First see if they have got their SEE evaluated yet
            if (ctx.hasGoodSee[a.getSource()][a.getDest()] == UNKNOWN) {
                ctx.hasGoodSee[a.getSource()][a.getDest()] = posutils::hasGoodSEE(pos, a)
                        ? YES : NO;
            }
            if (ctx.hasGoodSee[b.getSource()][b.getDest()] == UNKNOWN) {
                ctx.hasGoodSee[b.getSource()][b.getDest()] = posutils::hasGoodSEE(pos, b)
                                                             ? YES : NO;
            }
            // They have their SEE evaluated, compare
            bool aHasGoodSee = ctx.hasGoodSee[a.getSource()][a.getDest()] == YES;
            bool bHasGoodSee = ctx.hasGoodSee[b.getSource()][b.getDest()] == YES;

            if (aHasGoodSee && !bHasGoodSee) {
                return true;
            }
            if (!aHasGoodSee && bHasGoodSee) {
                return false;
            }
            // Both have the same SEE.
        }
        // Then come killer moves
        bool aIsKiller = isKillerMove(a, ctx.currPly);
        bool bIsKiller = isKillerMove(b, ctx.currPly);
        if (aIsKiller && !bIsKiller) {
            return true;
        }
        if (bIsKiller && !aIsKiller) {
            return false;
        }

        // Then history heuristic moves
        if constexpr (!NOISY) {
            Color us = a.getSourcePiece().getColor();

            int histA = m_History[us][a.getSource()][a.getDest()];
            int histB = m_History[us][b.getSource()][b.getDest()];
            if (histA > histB) {
                aScore += m_Scores.historyScore;
            } else {
                bScore += m_Scores.historyScore;
            }
        }
        else {
        }

        aScore += scoreMove<NOISY>(pos, a, ctx.currPly);
        bScore += scoreMove<NOISY>(pos, b, ctx.currPly);

        return aScore > bScore;
    }

    template <bool NOISY>
    int scoreMove(Position& pos, Move move, int currPly) const {
        int total = 0;

        // Check if killer move
        if (move == m_Killers[currPly][0] || move == m_Killers[currPly][1]) {
            //total += m_Scores.killerMoveScore;
        }

        if constexpr (NOISY) {
            // Noisy moves only
            if (move.is<MTM_CAPTURE>()) {
                total += m_Scores.captureBaseScore;
                total += m_Scores.mvvLva[move.getSourcePiece().getType()][move.getDestPiece().getType()]
                        * m_Scores.mvvLvaPctFactor / 100;
            }

            if (move.is<MTM_PROMOTION>()) {
                total += m_Scores.promotionBaseScore;
                total += m_Scores.promotionPieceTypeScore * getPointValue(move.getPromotionPiece());
            }
            return total;
        }
        // Quiet moves only
        if (move.is<MTM_CASTLES>()) {
            // Castling is usually a good option when available
            total += m_Scores.castlesScore;
        }

        // Hotmap delta
        total += getHotmapDelta(move) * m_Scores.placementDeltaMultiplier;

        total -= getPointValue(move.getSourcePiece().getType());

        return total;
    }

    template <bool NOISY>
    void sortMoves(Position& pos, int currPly, Move pvMove, MoveList::Iterator begin, MoveList::Iterator end) const {
        SortContext ctx;
        ctx.currPly = currPly;
        ctx.pvMove = pvMove;

        std::sort(begin, end, [this, &pos, currPly, ctx](Move a, Move b) {
            return compareMoves<NOISY>(pos, a, b, ctx);
        });
    }
};

}

#endif // LUNA_AI_AIMOVEGEN_H
