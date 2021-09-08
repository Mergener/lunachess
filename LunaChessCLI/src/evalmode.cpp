#include "evalmode.h"

#include "ai/movepicker.h"
#include "console.h"
#include "error.h"

namespace lunachess {

void EvalMode::doArgs(const std::vector<AppArg>& args) {
	for (auto& arg : args) {
		// Depth argument.
		if (arg.argName == "-depth") {
			runtimeAssert(arg.argParams.size() == 1, "Expected a depth for argument '-depth'");
			try {
				this->m_Depth = std::stoi(arg.argParams[0]);
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

static void displayScore(int score) {
	if (score < 0) {
		console::write("-");
	}

	console::write(std::abs(score) / 100);
	console::write(".");
	console::write(std::abs(score) % 100);
}

int EvalMode::run(const std::vector<AppArg>& args) {
	doArgs(args);

	ai::MovePicker movePicker;

	int score; Move move;
	std::tie(move, score) = movePicker.pickMove(m_Position, m_Depth);

	if (m_Position.getSideToMove() == Side::Black) {
		score *= -1;
	}

	console::write(m_Position);

	int scoreAbs = std::abs(score);
	if (scoreAbs < 20) {
		console::write("Equal position.");
	}
	else {
		if (score > 0) {
			console::write("White ");
		}
		else {
			console::write("Black ");
		}

		if (scoreAbs < 100) {
			console::write("is slightly better.");
		}
		else if (scoreAbs < 150) {
			console::write("is significantly better.");
		}
		else if (scoreAbs < ai::MovePicker::FORCED_MATE_THRESHOLD) {
			console::write("is winning.");
		}
		else {
			console::write("has forced mate in ");
			console::write(ai::MovePicker::MATE_IN_ONE_SCORE - scoreAbs);
			console::write(" ply.");
		}
	}
	if (scoreAbs < ai::MovePicker::FORCED_MATE_THRESHOLD) {
		console::write(" (");
		displayScore(score);
		console::write(")");
	}

	console::write("\nBest move: ");
	console::write(move);
	console::write("\n");

	return 0;
}

}