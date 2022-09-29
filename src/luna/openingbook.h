#ifndef  LUNA_OPENINGBOOK_H
#define  LUNA_OPENINGBOOK_H

#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "debug.h"
#include "position.h"

namespace lunachess {

class OpeningBook {
public:
    inline const std::vector<Move>& getMovesForPosition(ui64 posKey) const {
        return m_Moves.at(posKey);
    }
    inline const std::vector<Move>& getMovesForPosition(const Position& pos) const {
        return getMovesForPosition(pos.getZobrist());
    }

    Move getRandomMoveForPosition(ui64 posKey) const;
    inline Move getRandomMoveForPosition(const Position& pos) const {
        return getRandomMoveForPosition(pos.getZobrist());
    }

    void addMove(ui64 posKey, Move move);
    inline void addMove(const Position& pos, Move move) {
        addMove(pos.getZobrist(), move);
    }

    void deleteMove(ui64 posKey, Move move);
    inline void deleteMove(const Position& pos, Move move) {
        deleteMove(pos.getZobrist(), move);
    }

    void clearMovesFromPos(ui64 posKey);
    inline void clearMovesFromPos(const Position& pos) {
        clearMovesFromPos(pos.getZobrist());
    }

    void clear();

    static const OpeningBook& getDefault();

private:
    std::unordered_map<ui64, std::vector<Move>> m_Moves;
};

class OpeningBookBuilder {
public:
    inline OpeningBookBuilder& add(Move m) {
        LUNA_ASSERT(m_Pos.isMoveLegal(m), "Move must be legal");
        m_Book.addMove(m_Pos, m);
        return *this;
    }
    inline OpeningBookBuilder& add(Square src, Square dest, PieceType promTarget = PT_NONE) {
        return add(Move(m_Pos, src, dest, promTarget));
    }
    inline OpeningBookBuilder& add(std::string_view mv) {
        return add(Move(m_Pos, mv));
    }

    inline OpeningBookBuilder& push(Move m) {
        LUNA_ASSERT(m_Pos.isMoveLegal(m), "Move must be legal");
        m_Pos.makeMove(m);
        return *this;
    }
    inline OpeningBookBuilder& push(Square src, Square dest, PieceType promTarget = PT_NONE) {
        return push(Move(m_Pos, src, dest, promTarget));
    }
    inline OpeningBookBuilder& push(std::string_view mv) {
        return push(Move(m_Pos, mv));
    }

    inline OpeningBookBuilder& pushAndAdd(Move m) {
        add(m);
        push(m);
        return *this;
    }
    inline OpeningBookBuilder& pushAndAdd(Square src, Square dest, PieceType promTarget = PT_NONE) {
        return pushAndAdd(Move(m_Pos, src, dest, promTarget));
    }
    inline OpeningBookBuilder& pushAndAdd(std::string_view mv) {
        return pushAndAdd(Move(m_Pos, mv));
    }

    inline OpeningBookBuilder& pop() {
        m_Pos.undoMove();
        return *this;
    }

    inline const OpeningBook& get() const {
        return m_Book;
    }

    inline explicit OpeningBookBuilder(Position pos = Position::getInitialPosition())
        : m_Pos(std::move(pos)) {
    }

private:
    OpeningBook m_Book;
    Position m_Pos;
};

} // lunachess

#endif // LUNA_OPENINGBOOK_H
