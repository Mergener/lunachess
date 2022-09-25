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

    // Write all PGN tags
    for (const auto& pair: m_Tags) {
        addPgnTag(ss, pair.first, pair.second);
    }

    // Now write moves
    // Moves are preceded with their respective move number.
    // Games that start from a position might start with the color
    // to move being black, and in that case, we start with 'MOVNUM ...',
    // instead of 'MOVNUM .'
    if (!m_Moves.empty()) {
        Position pos = getStartingPosition();
        int plyCount = pos.getPlyCount();
        int moveNumber = plyCount / 2 + 1;
        if (plyCount % 2 == 0) {
            ss << moveNumber << ". ";
        } else {
            ss << moveNumber << "... ";
        }
        ss << m_Moves[0].toAlgebraic(pos) << ' ';
        pos.makeMove(m_Moves[0]);

        // Finally, write the moves
        for (int i = 1; i < m_Moves.size(); ++i) {
            if (i % args.pliesPerLine == 0) {
                ss << std::endl;
            }

            if (pos.getPlyCount() % 2 == 0) {
                moveNumber++;
                ss << moveNumber << ". ";
            }
            Move m = m_Moves[i];
            ss << m.toAlgebraic(pos) << ' ';
            pos.makeMove(m);
        }
    }

    // By the end, write the result
    auto result = getResultForWhite();
    if (result == RES_UNFINISHED) {
        ss << '*';
    }
    else if (isWin(result)) {
        ss << "1-0";
    }
    else if (isLoss(result)) {
        ss << "0-1";
    }
    else {
        ss << "1/2-1/2";
    }

    return ss.str();
}

void playGame(ChessGame& game,
              PlayerFunc playerWhite,
              PlayerFunc playerBlack,
              PlayGameArgs args,
              std::function<bool(const Position&)> stopCondition) {
    // Setup initial position
    Position pos = args.startingPosition;
    game.setStartingPosition(pos);

    // Setup time controls for both players
    TimeControl tcs[] = {
        args.timeControl[CL_WHITE],
        args.timeControl[CL_BLACK]
    };

    ChessResult result = pos.getResult(CL_WHITE, true);

    bool flagged = false;
    while (result == RES_UNFINISHED) {
        if (stopCondition(pos)) {
            // Stop was requested
            break;
        }

        Color color = pos.getColorToMove();
        auto& player = color == CL_WHITE ? playerWhite : playerBlack;

        auto timeBeforeMove = Clock::now();
        Move m = player(pos, tcs[color], tcs[getOppositeColor(color)]);
        if (m == MOVE_INVALID) {
            result = color == CL_WHITE ? RES_LOSS_RESIGN : RES_WIN_RESIGN;
            break;
        }

        auto elapsed = deltaMs(Clock::now(), timeBeforeMove);
        if (tcs[color].mode != TC_INFINITE && elapsed > tcs[color].time) {
            flagged = true;
        }
        else if (tcs[color].mode == TC_FISCHER) {
            tcs[color].time += tcs[color].increment - elapsed;
        }

        result = pos.getResult(CL_WHITE, !flagged);
        pos.makeMove(m);
        game.pushMove(m);
    }

    game.setResultForWhite(result);
}


}