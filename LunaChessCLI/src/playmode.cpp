#include "playmode.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <iomanip>

#include "console.h"
#include "error.h"

namespace lunachess {

void PlayMode::parseArgs(const std::vector<AppArg>& args) {
	for (auto& arg : args) {
		// Side argument. Choses the side in which the AI should play.
		// If set to 'both' the AI will play against itself.
		if (arg.argName == "-side") {
			runtimeAssert(arg.argParams.size() == 1, "Expected a side for argument '-side'.");

			std::string sideName = arg.argParams[0];

			if (sideName == "white") {
				this->m_BotSide = Side::White;
			}
			else if (sideName == "black") {
				this->m_BotSide = Side::Black;
			}
			else if (sideName == "both") {
				this->m_BotSide = Side::None;
			}
			else {
				runtimeAssert(false, "Invalid side '" + sideName + "'  for argument '-depth'.");
			}
		}
		else if (arg.argName == "--showeval") {
			this->m_ShowEval = true;
		}
		else if (arg.argName == "--showtime") {
			this->m_ShowTime = true;
		}
		// Depth argument.
		else if (arg.argName == "-depth") {
			runtimeAssert(arg.argParams.size() == 1, "Expected a depth for argument '-depth'");
			try {
				this->m_BotDepth = std::stoi(arg.argParams[0]);
			}
			catch (const std::exception& ex) {
				runtimeAssert(false, "Invalid depth for argument '-depth'.");
			}
		}
		// Initial position FEN argument
		else if (arg.argName == "-fen") {
			runtimeAssert(arg.argParams.size() == 1, "Expected one FEN string for argument '-fen'");

			std::optional<Position> fenPos = Position::fromFEN(arg.argParams[0]);
			if (!fenPos.has_value()) {
				runtimeAssert(false, std::string("Invalid FEN string: '") + arg.argParams[0] + "'");
			}

			m_Position = *fenPos;
		}
		else {
			runtimeAssert(false, "Unknown argument '" + arg.argName + "'");
		}
	}
}

static Square strToSquare(std::string_view s) {
	int file = s[0] - 'a';
	if (file < 0 || file > 7) {
		return SQ_INVALID;
	}

	int rank = s[1] - '1';
	if (rank < 0 || rank > 7) {
		return SQ_INVALID;
	}

	return rank * 8 + file;
}

static Move strToMove(std::string_view s, const Position& pos) {
	if (s.size() != 4 && s.size() != 5) {
		return MOVE_INVALID;
	}

	Square src = strToSquare(s.substr(0, 2));
	if (src == SQ_INVALID) {
		return MOVE_INVALID;
	}
	Square dest = strToSquare(s.substr(2, 2));
	if (dest == SQ_INVALID) {
		return MOVE_INVALID;
	}

	PieceType p = PieceType::None;
	if (s.size() == 5) {
		switch (s[4]) {
		case 'q':
			p = PieceType::Queen;
			break;

		case 'r':
			p = PieceType::Rook;
			break;

		case 'b':
			p = PieceType::Bishop;
			break;

		case 'n':
			p = PieceType::Knight;
			break;

		default:
			return MOVE_INVALID;
		}
	}

	return Move(pos, src, dest, p);
}

void PlayMode::humanTurn() {
	std::string s;
	// Read input from stdin
	std::cin >> s;

	if (s[0] != '!') {
		// User is playing a move
		Move move = strToMove(s, m_Position);
		if (move.invalid()) {
			console::write("Invalid move\n");
			return;
		}

		if (!m_Position.makeMove(move, true)) {
			console::write("Illegal move\n");
			return;
		}
	}
	else {
		// User is issuing a command.
	}
}

static void displayTime(ui64 miliseconds) {
    console::write(miliseconds / 1000);
    console::write(".");
    console::write(std::setfill('0'));
    console::write(std::setw(3));
    console::write(miliseconds % 1000);
    console::write("s");
}

void PlayMode::computerTurn() {
	Move move;
	int score;

	using namespace std::chrono;
	steady_clock clock;
	auto now = clock.now();

	std::tie(move, score) = m_MovePicker.pickMove(m_Position, m_BotDepth);
	if (m_Position.getSideToMove() == Side::Black) {
		score *= -1;
	}

	bool accepted = m_Position.makeMove(move, true);

	auto delta = clock.now() - now;
	ui64 miliseconds = delta.count() / 1000000;

	if (!accepted) {
		m_Playing = false;
	}
	else {
        m_TotalCpPlayTime += miliseconds;
        m_TotalCpMoves++;
		console::write(move);

		if (m_ShowEval) {
			console::write(" - eval: ");
			console::write(score / 100);
			console::write(".");
			console::write(std::setfill('0'));
			console::write(std::setw(2));
			console::write(std::abs(score) % 100);
		}

		if (m_ShowTime) {
            console::write(" - time: ");
            displayTime(miliseconds);
		}
		
		console::write('\n');
	}
}

void PlayMode::play() {
	m_Playing = true;
	MoveList legalMoves;
	
	while (m_Playing) {
		legalMoves.clear();
		m_Position.getLegalMoves(legalMoves);

		if (legalMoves.count() == 0) {
			if (m_Position.isCheck()) {
				if (m_Position.getSideToMove() == Side::White) {
					console::write("Checkmate. Black wins.\n");
				}
				else {
					console::write("Checkmate. White wins.\n");
				}

			}
			else {
				console::write("Draw by stalemate.\n");
			}

			m_Playing = false;
			break;
		}

		if (m_Position.is50moveRuleDraw()) {
			console::write("Draw by 50 move rule.");
			m_Playing = false;
			break;
		}

		if (m_Position.isRepetitionDraw()) {
			console::write("Draw by threefold repetition.");
			m_Playing = false;
			break;
		}

		if (m_Position.isInsufficientMaterialDraw()) {
			console::write("Draw by insufficient material.");
			m_Playing = false;
			break;
		}

		if (m_BotSide == Side::None) {
			// Computer is playing against itself
			computerTurn();
		}
		else if (m_Position.getSideToMove() == m_BotSide) {
			computerTurn();
		}
		else {
			humanTurn();
		}

	}

    if (m_ShowTime) {
        console::write("Total computer time: ");
        displayTime(m_TotalCpPlayTime);
        console::write(" (average ");
        displayTime(m_TotalCpPlayTime / m_TotalCpMoves);
        console::write(" per move)");
    }
}

int PlayMode::run(const std::vector<AppArg>& args) {
	parseArgs(args);
	play();
	return EXIT_SUCCESS;
}

}