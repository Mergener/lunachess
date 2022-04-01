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
    int promotionBaseScore = 5000;
    int promotionPieceTypeScore = 100;
    int captureBaseScore = 2000;
    int captureDeltaScore = 100;
    int castlesScore = 50;
    int killerMoveScore = 2000;
    int historyScore = 400;
    int placementDeltaMultiplier = 5;
};

/**
 * Generates and order moves for an alpha-beta pruning based move searcher.
 */
class AIMoveFactory {
public:
    int generateMoves(MoveList& ml, Position& pos, int currPly, Move pvMove = MOVE_INVALID);
    int generateNoisyMoves(MoveList& ml, Position& pos, int currPly, Move pvMove = MOVE_INVALID);

    void storeKillerMove(Move move, int ply);

    inline void storeHistory(Move move, int depth) {
        m_History[move.getSourcePiece().getColor()][move.getSource()][move.getDest()] += depth*depth;
    }

    AIMoveFactory() = default;
    inline AIMoveFactory(const MoveOrderingScores& scores)
        : m_Scores(scores) {
        resetHistory();
    }

    inline void resetHistory() {
        std::memset(m_History, 0, sizeof(m_History));
    }

private:
    MoveOrderingScores m_Scores;

    Move m_Killers[128][2];
    int m_History[CL_COUNT][SQ_COUNT][SQ_COUNT];

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

    template <bool NOISY>
    int scoreMove(Position& pos, Move move, int currPly) const {
        int total = 0;

        // Check if killer move
        if (move == m_Killers[currPly][0] || move == m_Killers[currPly][1]) {
            total += m_Scores.killerMoveScore;
        }

        if constexpr (NOISY) {
            // Noisy moves only
            if (move.is<MTM_CAPTURE>()) {
                total += m_Scores.captureBaseScore;
                total += m_Scores.captureDeltaScore * getPointValue(move.getDestPiece().getType());
                total -= m_Scores.captureDeltaScore * getPointValue(move.getSourcePiece().getType());
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
        std::sort(begin, end, [this, &pos, currPly, pvMove](Move a, Move b) {

            if constexpr (!NOISY) {
                if (a == pvMove) {
                    return true;
                }
                if (b == pvMove) {
                    return false;
                }
            }

            int aScore = scoreMove<NOISY>(pos, a, currPly);
            int bScore = scoreMove<NOISY>(pos, b, currPly);

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

            return aScore > bScore;
        });
    }
};

}

#endif // LUNA_AI_AIMOVEGEN_H
