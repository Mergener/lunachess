#include "chessgame.h"

#include <ostream>
#include <sstream>

#include "clock.h"

namespace lunachess {

Position ChessGame::getFinalPosition() {
    Position ret = m_StartPos;

    for (Move m: m_Moves) {
        ret.makeMove(m);
    }

    return ret;
}

static void addPgnTag(std::ostream& stream,
                      std::string_view tagName, 
                      std::string_view tagValue) {
    stream << "[" << tagName << " \"" << tagValue << "\"]" << std::endl;
}

std::string ChessGame::toPgn(ToPgnArgs args) const {
    std::stringstream ss;

    for (const auto& pair: m_Tags) {
        addPgnTag(ss, pair.first, pair.second);
    }

    Position pos = getStartingPosition();
    int plyCount = pos.getPlyCount();
    int moveNumber = plyCount / 2 + 1;
    if (plyCount % 2 == 0) {
        ss << moveNumber << ". ";
    }
    else {
        ss << moveNumber << "... ";
    }

    for (int i = 1; i < m_Moves.size(); ++i) {
        if (pos.getPlyCount() % 2 == 0) {
            moveNumber++;
            ss << moveNumber << ". ";
        }
        Move m = m_Moves[i];
        pos.makeMove(m);
        ss << m << ' ';

        if (i % args.pliesPerLine == 0) {
            ss << std::endl;
        }
    }

    return ss.str();
}

void playGame(ChessGame& game,
              PlayerFunc playerWhite,
              PlayerFunc playerBlack,
              PlayGameArgs args = PlayGameArgs()) {
    Position pos = game.getStartingPosition();

    TimeControl tcs[] = {
        args.timeControl[CL_WHITE],
        args.timeControl[CL_BLACK]
    };

    ChessResult result = pos.getResult(CL_WHITE, true);

    bool flagged = false;
    while (result == RES_UNFINISHED) {
        Color color = pos.getColorToMove();
        auto& player = color == CL_WHITE ? playerWhite : playerBlack;

        auto timeBeforeMove = Clock::now();
        Move m = player(tcs[color], tcs[getOppositeColor(color)]);
        auto elapsed = deltaMs(Clock::now(), timeBeforeMove);
        if (tcs[color].mode != TC_INFINITE && elapsed > tcs[color].time) {
            flagged = true;
        }
        else if (tcs[color].mode == TC_FISCHER) {
            tcs[color].time += tcs[color].increment - elapsed;
        }

        result = pos.getResult(CL_WHITE, !flagged);
        pos.makeMove(m);
    }
}


}