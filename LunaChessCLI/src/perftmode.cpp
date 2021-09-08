#include "perftmode.h"

#include <chrono>
#include <iomanip>

#include "console.h"
#include "error.h"

namespace lunachess {

void PerftMode::doArgs(const std::vector<AppArg>& args) {
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
		else if (arg.argName == "--pseudo") {
			m_PseudoLegal = true;
		}
		else {
			runtimeAssert(false, "Unknown argument '" + arg.argName + "'");
		}
	}
}


static ui64 perft(Position& pos, int depth) {
	MoveList moveList;
	int n, i;
	ui64 nodes = 0;

	n = pos.getLegalMoves(moveList);

	if (depth == 1) {
		return static_cast<ui64>(n);
	}
	depth--;
	for (i = 0; i < n; i++) {
		pos.makeMove(moveList[i]);
		nodes += perft(pos, depth);
		pos.undoMove(moveList[i]);
	}
	return nodes;
}

static ui64 perftPseudo(Position& pos, int depth) {
	MoveList moveList;
	int n, i;
	ui64 nodes = 0;

	n = pos.getPseudoLegalMoves(moveList);

	if (depth == 1) {
		return static_cast<ui64>(n);
	}
	depth--;
	for (i = 0; i < n; i++) {
		pos.makeMove(moveList[i]);
		nodes += perftPseudo(pos, depth);
		pos.undoMove(moveList[i]);
	}
	return nodes;
}

static ui64 perftMain(const Position& pos, bool pseudoLegal, int depth) {
	Position repl = pos;

	if (pseudoLegal) {
		return perftPseudo(repl, depth);
	}
	return perft(repl, depth);
}

int PerftMode::run(const std::vector<AppArg>& args) {
	using namespace std::chrono;

	steady_clock clock;
	
	doArgs(args);

	auto now = clock.now();

	ui64 res = perftMain(m_Position, m_PseudoLegal, m_Depth);

	auto delta = duration_cast<milliseconds>(clock.now() - now);
	ui64 milliseconds = delta.count();
	double secondsf = static_cast<double>(milliseconds) / 1000;

	console::write("Result (depth ");
	console::write(m_Depth);
	console::write("): ");
	console::write(res);
	console::write("\nElapsed time: ");
	console::write(milliseconds / 1000);
	console::write(".");
	console::write(std::setfill('0'));
	console::write(std::setw(3));
	console::write(milliseconds % 1000);
	console::write("s (");
	console::write(static_cast<ui64>(res / secondsf));
	console::write(" N/s)\n");	

	return EXIT_SUCCESS;
}

}