#ifndef LUNA_MOVECURSOR_H
#define LUNA_MOVECURSOR_H

#include <cstring>

#include "../position.h"
#include "../movegen.h"
#include "../utils.h"
#include "../staticanalysis.h"

namespace lunachess::ai {

enum MoveCursorStage {

    MCS_NOT_STARTED,
    MCS_HASH_MOVE,
    MCS_PROM_CAPTURES,
    MCS_PROMOTIONS,
    MCS_GOOD_CAPTURES,
    MCS_EN_PASSANTS,
    MCS_KILLERS,
    MCS_BAD_CAPTURES,
    MCS_QUIET,
    MCS_END,

};
static_assert(MCS_BAD_CAPTURES > MCS_GOOD_CAPTURES, "Bad captures must come after good captures.");
static_assert(MCS_QUIET > MCS_KILLERS, "Quiet moves must come after killer moves.");

class MoveOrderingData {
public:
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

    inline void storeCounterMove(Move lastMove, Move counterMove) {
        m_CounterMoves[lastMove.getSource()][lastMove.getDest()] = counterMove;
    }

    inline bool isCounterMove(Move lastMove, Move counterMove) const {
        return m_CounterMoves[lastMove.getSource()][lastMove.getDest()] == counterMove;
    }

    inline int getMoveHistory(Move move) const {
        return m_History[move.getSourcePiece().getColor()][move.getSource()][move.getDest()];
    }

    inline Move getKillerMove(int ply, int index) const {
        return m_Killers[ply][index];
    }

    int scoreQuietMove(Move move, const Position& pos) const;

    inline void resetCountermoves() {
        std::memset(m_CounterMoves, 0, sizeof(m_CounterMoves));
    }

    inline void resetHistory() {
        std::memset(m_History, 0, sizeof(m_History));
    }

    inline void resetKillers() {
        std::memset(m_Killers, 0, sizeof(m_Killers));
    }

    inline void resetAll() {
        resetKillers();
        resetHistory();
        resetCountermoves();
    }

    inline MoveOrderingData() {
        resetAll();
    }

private:
    Move m_Killers[128][2];
    Move m_CounterMoves[SQ_COUNT][SQ_COUNT];
    int  m_History[CL_COUNT][SQ_COUNT][SQ_COUNT] = {};
};

template <bool NOISY_ONLY = false>
class MoveCursor {
public:
    Move next(const Position& pos,
              const MoveOrderingData& moveOrderingData,
              int ply,
              Move hashMove = MOVE_INVALID) {
        // We start by checking if we still have more moves to
        // use from the current stage. If we don't, we need to
        // advance to the next stage.
        while (m_Remaining <= 0) {
            if (m_Stage >= MCS_END) {
                // We are done with this position.
                return MOVE_INVALID;
            }
            advanceStage(pos, moveOrderingData, ply, hashMove);
        }
        m_Remaining--;

        Move move;
        switch (m_Stage) {
            case MCS_HASH_MOVE:
                return hashMove;

            case MCS_QUIET:
                move = nextQuiet(pos, moveOrderingData, ply);
                break;

            default:
                move = *m_Iter++;
                break;
        }
        if (move == MOVE_INVALID || !pos.isMoveLegal(move) || move == hashMove) {
            // Illegal move, try the next one.
            // The call to next is likely going to get
            // tail-call optimized.
            return next(pos, moveOrderingData, ply);
        }

        return move;
    }

private:
    MoveCursorStage m_Stage = MCS_NOT_STARTED;
    MoveList m_Moves;
    MoveList::Iterator m_Iter = m_Moves.begin();
    MoveList::Iterator m_SimpleCapturesBegin;
    int m_NGoodCaptures = 0;
    int m_NBadCaptures  = 0;

    /** Number of moves remaining in the current stage. */
    int m_Remaining = 0;

    void advanceStage(const Position& pos,
                      const MoveOrderingData& moveOrderingData,
                      int ply,
                      Move hashMove = MOVE_INVALID) {
        m_Stage = static_cast<MoveCursorStage>(m_Stage + 1);

        switch (m_Stage) {
            case MCS_HASH_MOVE:
                m_Remaining = hashMove != MOVE_INVALID;
                break;

            case MCS_PROM_CAPTURES:
                m_Iter      = m_Moves.end();
                m_Remaining = movegen::generate<bits::makeMask<MT_PROMOTION_CAPTURE>(), PTM_ALL, true>(pos, m_Moves);
                break;

            case MCS_PROMOTIONS:
                m_Iter      = m_Moves.end();
                m_Remaining = movegen::generate<bits::makeMask<MT_SIMPLE_PROMOTION>(), PTM_ALL, true>(pos, m_Moves);
                break;

            case MCS_GOOD_CAPTURES:
                m_Iter = m_Moves.end();
                generateSimpleCaptures(pos, moveOrderingData);
                m_Remaining = m_NGoodCaptures;
                break;

            case MCS_EN_PASSANTS:
                m_Iter      = m_Moves.end();
                m_Remaining = movegen::generate<bits::makeMask<MT_EN_PASSANT_CAPTURE>(), BIT(PT_PAWN), true>(pos, m_Moves);
                break;

            case MCS_KILLERS: {
                if constexpr (NOISY_ONLY) {
                    advanceStage(pos, moveOrderingData, hashMove);
                    return;
                }

                m_Iter      = m_Moves.end();
                m_Remaining = 0;
                for (int i = 0; i < 2; ++i) {
                    Move killer = moveOrderingData.getKillerMove(ply, i);

                    if (pos.isMovePseudoLegal(killer)) {
                        m_Remaining++;
                        m_Moves.add(killer);
                    }
                }
                break;
            }

            case MCS_BAD_CAPTURES:
                m_Iter      = m_SimpleCapturesBegin + m_NGoodCaptures;
                m_Remaining = m_NBadCaptures;
                break;

            case MCS_QUIET:
                if constexpr (NOISY_ONLY) {
                    advanceStage(pos, moveOrderingData, hashMove);
                    return;
                }

                m_Iter      = m_Moves.end();
                m_Remaining = generateQuietMoves(pos, moveOrderingData);
                break;

            default:
                m_Remaining = 0;
                break;
        }
    }

    Move nextQuiet(const Position& pos,
                   const MoveOrderingData& moveOrderingData,
                   int ply) {
        // Skip killer moves
        while (moveOrderingData.isKillerMove(*m_Iter, ply) &&
               m_Remaining > 0) {
            m_Iter++;
            m_Remaining--;
        }
        return *m_Iter++;
    }

    /**
     * Generates simple captures, sorts them and sets m_NGoodCaptures, m_NBadCaptures
     * and m_SimpleCapturesBegin.
     */
    void generateSimpleCaptures(const Position& pos,
                                const MoveOrderingData& moveOrderingData) {
        m_SimpleCapturesBegin = m_Moves.end();
        movegen::generate<bits::makeMask<MT_SIMPLE_CAPTURE>(), PTM_ALL, true>(pos, m_Moves);

        bool seeTable[SQ_COUNT][SQ_COUNT];
        for (auto it = m_SimpleCapturesBegin; it != m_Moves.end(); ++it) {
            Move m = *it;
            if (staticanalysis::hasGoodSEE(pos, m)) {
                seeTable[m.getSource()][m.getDest()] = true;
                m_NGoodCaptures++;
            }
            else {
                seeTable[m.getSource()][m.getDest()] = false;
                m_NBadCaptures++;
            }
        }

        // Sort the captures in see > mvv-lva order
        utils::insertionSort(m_SimpleCapturesBegin, m_Moves.end(), [&seeTable](Move a, Move b) {
            bool aHasGoodSEE = seeTable[a.getSource()][a.getDest()];
            bool bHasGoodSEE = seeTable[b.getSource()][b.getDest()];
            if (aHasGoodSEE && !bHasGoodSEE) {
                return true;
            }
            if (!aHasGoodSEE && bHasGoodSEE) {
                return false;
            }

            return compareMvvLva(a, b);
        });
    }

    /** Generates and sorts quiet moves. Returns the number of generated quiet moves. */
    int generateQuietMoves(const Position& pos,
                           const MoveOrderingData& moveOrderingData) {
        auto quietBegin = m_Moves.end();
        int ret = movegen::generate<MTM_QUIET, PTM_ALL, true>(pos, m_Moves);

        int scores[SQ_COUNT][SQ_COUNT];
        for (auto it = quietBegin; it != m_Moves.end(); ++it) {
            Move m = *it;
            scores[m.getSource()][m.getDest()] = moveOrderingData.scoreQuietMove(m, pos);
        }

        utils::insertionSort(quietBegin, m_Moves.end(), [&scores](Move a, Move b) {
            int aScore = scores[a.getSource()][a.getDest()];
            int bScore = scores[b.getSource()][b.getDest()];
            return aScore > bScore;
        });

        return ret;
    }

    static bool compareMvvLva(Move a, Move b) {
        constexpr int MVV_LVA[PT_COUNT][PT_COUNT] {
                /*         x-    xP    xN    xB    xR    xQ    xK  */
                /* -- */ { 0,    0,    0,    0,    0,    0,    0   },
                /* Px */ { 0,   105,  205,  305,  405,  505,  9999 },
                /* Nx */ { 0,   104,  204,  304,  404,  504,  9999 },
                /* Bx */ { 0,   103,  203,  303,  403,  503,  9999 },
                /* Rx */ { 0,   102,  202,  302,  600,  502,  9999 },
                /* Qx */ { 0,   101,  201,  301,  401,  501,  9999 },
                /* Kx */ { 0,   100,  200,  300,  400,  500,  9999 },
        };

        return MVV_LVA[a.getSourcePiece().getType()][a.getDestPiece().getType()] >
               MVV_LVA[b.getSourcePiece().getType()][b.getDestPiece().getType()];
    }
};

}

#endif // LUNA_MOVECURSOR_H
