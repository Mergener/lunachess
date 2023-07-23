#ifndef LUNA_MOVECURSOR_H
#define LUNA_MOVECURSOR_H

#include <cstring>

#include "../position.h"
#include "../movegen.h"
#include "../utils.h"
#include "../staticanalysis.h"

namespace lunachess::ai {

enum MoveCursorStage {

    MCS_HASH_MOVE,
    MCS_PROM_CAPTURES,
    MCS_PROMOTIONS,
    MCS_CAPTURES,
    MCS_QUIET

};

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

int getQuietMoveScore(Move move);

template <bool NOISY_ONLY = false>
class MoveCursor {
    inline static constexpr MoveCursorStage LAST_STAGE =
            NOISY_ONLY
                ? MCS_CAPTURES
                : MCS_QUIET;

public:
    Move next(const Position& pos,
              const MoveOrderingData& moveOrderingData,
              int ply,
              Move hashMove = MOVE_INVALID) {

        while (m_Iter == m_Moves.end()) {
            if (m_Stage > LAST_STAGE) {
                return MOVE_INVALID;
            }
            if (m_Stage == MCS_HASH_MOVE && hashMove != MOVE_INVALID) {
                m_Stage = static_cast<MoveCursorStage>(m_Stage + 1);
                return hashMove;
            }
            generate(pos, moveOrderingData, ply);
            m_Stage = static_cast<MoveCursorStage>(m_Stage + 1);
        }

        // Prevent hashMove from being searched more than once
        Move move = *m_Iter++;
        if (move == hashMove || !pos.isMoveLegal(move)) {
            return next(pos, moveOrderingData, ply, hashMove);
        }

        return move;
    }

private:
    MoveList m_Moves;
    MoveList::Iterator m_Iter = m_Moves.begin();
    MoveCursorStage m_Stage = MCS_HASH_MOVE;

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

    void generate(const Position& pos,
                  const MoveOrderingData& moveOrderingData,
                  int ply) {
        auto stageBegin = m_Moves.end();

        switch (m_Stage) {
            case MCS_PROM_CAPTURES: {
                movegen::generate<BIT(MT_PROMOTION_CAPTURE), PTM_ALL, true>(pos, m_Moves);
                utils::insertionSort(m_Moves.begin(), m_Moves.end(), [this](Move a, Move b) {
                    return compareMvvLva(a, b);
                });
                break;
            }

            case MCS_PROMOTIONS: {
                movegen::generate<BIT(MT_SIMPLE_PROMOTION), PTM_ALL, true>(pos, m_Moves);
                break;
            }

            case MCS_CAPTURES: {
                movegen::generate<BIT(MT_SIMPLE_CAPTURE), PTM_ALL, true>(pos, m_Moves);

                // Compute the SEE for each simple capture
                bool seeTable[SQ_COUNT][SQ_COUNT];
                for (auto it = stageBegin; it != m_Moves.end(); ++it) {
                    Move m = *it;
                    seeTable[m.getSource()][m.getDest()] = staticanalysis::hasGoodSEE(pos, m);
                }

                // Sort the captures in see > mvv-lva order
                utils::insertionSort(stageBegin, m_Moves.end(), [&seeTable](Move a, Move b) {
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

                movegen::generate<BIT(MT_EN_PASSANT_CAPTURE), BIT(PT_PAWN), true>(pos, m_Moves);
                break;
            }

            case MCS_QUIET: {
                movegen::generate<MTM_QUIET, PTM_ALL, true>(pos, m_Moves);
                utils::insertionSort(stageBegin, m_Moves.end(), [this, &pos, &moveOrderingData, ply](Move a, Move b) {
                    // Killer move heuristic
                    bool aIsKiller = moveOrderingData.isKillerMove(a, ply);
                    bool bIsKiller = moveOrderingData.isKillerMove(b, ply);
                    if (aIsKiller && !bIsKiller) {
                        return true;
                    }
                    else if (!aIsKiller && bIsKiller) {
                        return false;
                    }

                    // Countermove Heuristic
                    Move lastMove = pos.getLastMove();
                    bool aIsCountermove = moveOrderingData.isCounterMove(lastMove, a);
                    bool bIsCountermove = moveOrderingData.isCounterMove(lastMove, b);
                    if (aIsCountermove && !bIsCountermove) {
                        return true;
                    }
                    else if (!aIsCountermove && bIsCountermove) {
                        return false;
                    }

                    // History heuristic
                    int aHist = moveOrderingData.getMoveHistory(a);
                    int bHist = moveOrderingData.getMoveHistory(b);
                    if (aHist > bHist) {
                        return true;
                    }
                    if (aHist < bHist) {
                        return false;
                    }

                    return getQuietMoveScore(a) > getQuietMoveScore(b);
                });
            }

            default:
                break;
        }

    }
};

}

#endif // LUNA_MOVECURSOR_H
