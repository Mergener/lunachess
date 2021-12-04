#include "ucimode.h"

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>

#include "console.h"

#include "core/strutils.h"

namespace lunachess {

template<typename T>
static bool isReady(std::future<T> const& f) {
	return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

int UCIMode::run(const std::vector<AppArg>& args) {
	m_Running = true;

	Position pos = Position::fromFEN("8/8/k4P2/6K1/8/8/1p6/8 b - - 0 1").value();
	std::cout << bitboards::getPieceAttacks(BLACK_PAWN, pos.getCompositeBitboard(), SQ_B2);

	return uciLoop();
}

bool UCIMode::pollInput(std::string& input) {
	if (m_InputQueue.size() == 0) {
		return false;
	}

	m_InputQueueLock.lock();
	input = m_InputQueue.front();
	m_InputQueue.pop();
	m_InputQueueLock.unlock();

	return true;
}

int UCIMode::uciLoop() {
	std::vector<std::string_view> args;

	auto inputThreadFunc = [this]() {
		// Read all input from stdin and add to the input queue
		while (m_Running) {
			std::string input;
			std::getline(std::cin, input);

			m_InputQueueLock.lock();
			m_InputQueue.push(input);
			m_InputQueueLock.unlock();
		}
	};
	std::thread inputThread(inputThreadFunc);

	while (m_Running) {
		std::string input;
		// Read all pending input
		while (pollInput(input)) {
			// We got an input, process it
			strutils::reduceWhitespace(input);
			strutils::split(input, args, " ");

			std::string cmd = std::string(args[0]);
			args.erase(args.begin()); // args[0] is the command itself

			auto cmdIt = m_Commands.find(cmd);
			if (cmdIt != m_Commands.end()) {
				auto cmdFunc = cmdIt->second;

				// Execute command
				cmdFunc(args);
			}
			else {
				std::cerr << "Unknown command." << std::endl;
			}
		}

		// Manage background tasks
		for (int i = m_Tasks.size() - 1; i >= 0; --i) {
			if (isReady(*(m_Tasks[i]))) {
				m_Tasks.erase(m_Tasks.begin() + i);
			}
		}

		// If background tasks finished, we are ready. Notify the GUI
		// if it has previously requested something.
		if (m_PendingIsready && m_Tasks.size() == 0) {
			m_PendingIsready = false;
			std::cout << "readyok" << std::endl;
		}

		// Cleanup
		args.clear();
	}

	inputThread.detach();

	return 0;
}

void UCIMode::startTask(std::function<void()> task) {
	auto future = std::make_shared<std::future<void>>(std::async(task));
	m_Tasks.push_back(future);
}

void UCIMode::cmdUci(const CmdArgs &args) {
	std::cout << "id name LunaChess" << std::endl;
	std::cout << "id author Thomas Mergener" << std::endl;
	std::cout << "uciok" << std::endl;
}

void UCIMode::cmdIsready(const CmdArgs& args) {
	m_PendingIsready = true;
}

void UCIMode::cmdPosition(const CmdArgs& args) {
	if (args.size() == 0) {
		std::cerr << "Expected either a FEN string or 'startpos' as argument for 'position' command." << std::endl;
		return;
	}

	std::string_view posStr = args[0];
	if (posStr == "startpos") {
		m_Position = Position::getInitialPosition();
	}
	else {
		auto opt = Position::fromFEN(posStr);
		if (!opt.has_value()) {
			std::cerr << "Invalid fen string '" << posStr << "'\n";
			return;
		}
		m_Position = opt.value();
	}

	if (args.size() == 1) {
		return;
	}

	// We still have startmoves
	if (args[1] == "moves") {
		for (int i = 2; i < args.size(); ++i) {
			Move move = Move::fromLongAlgebraic(m_Position, args[i]);

			m_Position.makeMove(move);
		}
	}
}

void UCIMode::cmdUcinewgame(const CmdArgs &args) {
}

void UCIMode::cmdGo(const CmdArgs &args) {
	// Parse args
	for (int i = 0; i < args.size(); ++i) {
		const auto& arg = args[i];

		if (arg == "depth") {
			i++;

			if (i >= args.size()) {
				std::cerr << "Expected a depth for argument 'depth'." << std::endl;
				return;
			}

			int depth = std::stoi(std::string(args[i]));
			m_Depth = 6;
		}
	}
}

void UCIMode::cmdQuit(const CmdArgs &args) {
	m_Running = false;
}

UCIMode::UCIMode() {
	// Setup commands dictionary
	m_Commands["uci"] = [this](const CmdArgs& args) { cmdUci(args); };
	m_Commands["isready"] = [this](const CmdArgs& args) { cmdIsready(args); };
	m_Commands["ucinewgame"] = [this](const CmdArgs& args) { cmdUcinewgame(args); };
	m_Commands["position"] = [this](const CmdArgs& args) { cmdPosition(args); };
	m_Commands["go"] = [this](const CmdArgs& args) { cmdGo(args); };
	m_Commands["quit"] = [this](const CmdArgs& args) { cmdQuit(args); };
}

}