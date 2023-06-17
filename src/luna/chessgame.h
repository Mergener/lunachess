#ifndef LUNA_CHESSGAME_H
#define LUNA_CHESSGAME_H

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "position.h"

namespace lunachess {

struct ToPgnArgs {
    int pliesPerLine = 4;
};

class ChessGame {
public:
    inline void clearTags() {
        m_Tags.clear();
    }

    inline void setTag(std::string tag, std::string value) {
        m_Tags[tag] = value;
    }

    inline const std::string& getTagValue(const std::string& str) const {
        return m_Tags.at(str);
    }

    inline const Position& getStartingPosition() const {
        return m_StartPos;
    }

    inline void setStartingPosition(const Position& pos) {
        m_StartPos = pos;
    }

    inline ChessResult getResultForWhite() const {
        return m_Res;
    }

    inline void setResultForWhite(ChessResult res) {
        m_Res = res;
    }

    Position getFinalPosition();

    inline void pushMove(Move m) {
        if (m == MOVE_INVALID) {
            return;
        }
        m_Moves.push_back(m);
    }

    inline void popMove() {
        m_Moves.pop_back();
    }

    inline void clearMoves() {
        m_Moves.clear();
    }

    std::string toPgn(ToPgnArgs args = ToPgnArgs()) const;

    static std::optional<ChessGame> fromPGN(std::string_view pgn);

    ChessGame() = default;

    template <typename TIter>
    inline ChessGame(TIter movesBegin, TIter movesEnd, Position startPos = Position::getInitialPosition())
        : m_StartPos(std::move(startPos)), m_Moves(movesBegin, movesEnd) {
    }

private:
    Position m_StartPos;
    std::vector<Move> m_Moves;
    ChessResult m_Res = RES_UNFINISHED;
    std::unordered_map<std::string, std::string> m_Tags;
};

using PlayerFunc = std::function<Move(const Position& pos, TimeControl ourTC, TimeControl theirTC)>;
struct PlayGameArgs {
    Position startingPosition = Position::getInitialPosition();
    TimeControl timeControl[CL_COUNT] = { TimeControl(180000, 2000, TC_FISCHER), TimeControl(180000, 2000, TC_FISCHER) };
};

void playGame(ChessGame& game,
              PlayerFunc playerWhite,
              PlayerFunc playerBlack,
              PlayGameArgs args = PlayGameArgs(),
              std::function<bool(const Position&)> stopCondition = [](const auto& p) { return false; });

} // lunachess

#endif // LUNA_CHESSGAME_H