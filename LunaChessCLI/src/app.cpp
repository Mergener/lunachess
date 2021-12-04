#include "app.h"

#include <iostream>

#include "appmode.h"
#include "error.h"

#include "lunachess.h"

// Appmodes:
#include "perftmode.h"
#include "playmode.h"
#include "evalmode.h"
#include "ucimode.h"
//

namespace lunachess {

static AppMode* makeAppMode(int argc, char* argv[]) {
	// Assumes argc > 1
	std::string modeName = argv[1];
	if (modeName == "play") {
		return new PlayMode();
	}
	else if (modeName == "eval") {
		return new EvalMode();
	}
	else if (modeName == "perft") {
		return new PerftMode();
	}
	else if (modeName == "uci") {
		return new UCIMode();
	}
	else {
		runtimeAssert(false, "Unknown execution mode.");
	}
	return nullptr;
}

static std::vector<AppArg> makeArgs(int argc, char* argv[]) {
	std::vector<AppArg> ret;

	int i = 2;
	while (i < argc) {
		AppArg arg;

		arg.argName = argv[i];

		i++;

		while ((i < argc) && argv[i][0] != '-') {
			arg.argParams.push_back(argv[i]);

			i++;
		}
		ret.push_back(arg);
	}

	return ret;
}

}

int main(int argc, char* argv[]) {
	using namespace lunachess;

	lunachess::initialize();

	AppMode* mode;
	if (argc == 1) {
		// Assumed mode is play mode.
		mode = new PlayMode();
	}
	else {
		mode = makeAppMode(argc, argv);
	}

	auto args = makeArgs(argc, argv);

	int ret = mode->run(args);

	delete mode;

	return ret;
}