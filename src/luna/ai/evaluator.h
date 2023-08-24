#ifndef LUNA_AI_EVALUATOR_H
#define LUNA_AI_EVALUATOR_H

#include "../position.h"

namespace lunachess::ai {

class Evaluator {
public:
    /**
     * Returns the score, in millipawns and in the current color to move's perspective,
     * for the current evaluation position.
     */
    virtual i32 evaluate() const = 0;
    virtual i32 getDrawScore() const = 0;

    inline const Position& getPosition() const { return m_Pos; }

    /**
     * Sets the evaluation position.
     */
    inline void setPosition(const Position& pos) {
        m_Pos = pos;
        onSetPosition(m_Pos);
    }

    /**
     * Makes a move on the evaluation position.
     */
    inline void makeMove(Move move) {
        m_Pos.makeMove(move);
        onMakeMove(move);
    }

    /**
     * Undoes a move on the evaluation position.
     */
    inline void undoMove() {
        auto move = m_Pos.getLastMove();
        m_Pos.undoMove();
        onUndoMove(move);
    }

    /**
     * Makes a null move on the evaluation position.
     */
    inline void makeNullMove() {
        m_Pos.makeNullMove();
        onMakeNullMove();
    }

    /**
     * Undoes a null move on the evaluation position.
     */
    inline void undoNullMove() {
        m_Pos.undoNullMove();
        onUndoNullMove();
    }
    virtual ~Evaluator() = default;

protected:
    inline virtual void onSetPosition(const Position& pos) {}
    inline virtual void onMakeMove(Move move) {}
    inline virtual void onUndoMove(Move move) {}
    inline virtual void onMakeNullMove() {}
    inline virtual void onUndoNullMove() {}

private:
    Position m_Pos;
};

}

#endif // LUNA_AI_EVALUATOR_H