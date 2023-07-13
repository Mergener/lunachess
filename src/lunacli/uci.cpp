#include "uci.h"

#include <cstdlib>
#include <filesystem>
#include <future>
#include <functional>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include <lunachess.h>

namespace lunachess {

using CommandArgs = std::vector<std::string_view>;

enum UCIState {
    IDLE,
    BUSY,
    STOPPING
};

struct UCIContext {
    // Chess state
    Position pos = Position::getInitialPosition();

    // UCI settings
    bool debugMode = false;
    int multiPvCount = 1;

    // Internal state
    UCIState state = IDLE;

    // Search settings
    ai::AlphaBetaSearcher searcher;
    bool useOpBook = false;

    // HCE settings
    std::shared_ptr<ai::HandCraftedEvaluator> hce = std::make_shared<ai::HandCraftedEvaluator>();
    const ai::HCEWeightTable* hceWeights = ai::getDefaultHCEWeights();
};

using UCICommandFunction = std::function<void(UCIContext&, const CommandArgs&)>;

struct Command {
    int minExpectedArgs = 0;
    bool exactArgsCount = true;
    UCICommandFunction function;

    Command(UCICommandFunction func, int minExpectedArgs, bool exactArgsCount = true)
            : function(func), minExpectedArgs(minExpectedArgs), exactArgsCount(exactArgsCount) {}

    Command() = default;

    Command(Command&& other) = default;

    Command(const Command& other) = default;

    Command& operator=(const Command& other) = default;

    ~Command() = default;
};

static void errorWrongArg(std::string_view cmdName, std::string_view wrongArg) {
    std::cerr << "Unexpected argument '" << wrongArg << "' for command '" << cmdName << "'." << std::endl;
}

//
// UCI Commands:
//

static void displayOption(UCIContext& ctx, std::string_view optName,
                          std::string_view optType, std::string_view defaultVal = "",
                          std::string_view minVal = "", std::string_view maxVal = "") {
    std::cout << "option name " << optName << " type " << optType;

    if (!defaultVal.empty()) {
        std::cout << " default " << defaultVal;
    }
    if (!minVal.empty()) {
        std::cout << " min " << minVal;
    }
    if (!maxVal.empty()) {
        std::cout << " max " << maxVal;
    }

    std::cout << std::endl;
}

static void cmdUci(UCIContext& ctx, const CommandArgs& args) {
    std::cout << "id name LunaChess" << std::endl;
    std::cout << "id author Thomas Mergener" << std::endl;
    displayOption(ctx, "MultiPV", "spin", "1", "1", "500");
    displayOption(ctx, "Hash", "spin", strutils::toString(ai::TranspositionTable::DEFAULT_SIZE_MB), "1", "1048576");
    displayOption(ctx, "UseOwnBook", "check", "false");

    std::cout << "uciok" << std::endl;
}

static void cmdQuit(UCIContext& ctx, const CommandArgs& args) {
    ctx.state = STOPPING;
    std::exit(0);
}

static void cmdDebug(UCIContext& ctx, const CommandArgs& args) {
    if (args[0] == "on") {
        ctx.debugMode = true;
    }
    else if (args[0] == "off") {
        ctx.debugMode = false;
    }
    else {
        errorWrongArg("debug", args[0]);
    }
}

static void cmdIsready(UCIContext& ctx, const CommandArgs& args) {
    std::cout << "readyok" << std::endl;
}

static void processOption(UCIContext& ctx, std::string_view option, std::string_view value) {
    if (option == "MultiPV") {
        int count;
        if (strutils::tryParseInteger(value, count)) {
            ctx.multiPvCount = count;
        }
    }
    else if (option == "Hash") {
        size_t size;
        if (strutils::tryParseInteger(value, size)) {
            ctx.searcher.getTT().resize(size * 1024 * 1024);
        }
    }
    else if (option == "UseOwnBook") {
        if (value == "true") {
            ctx.useOpBook = true;
        }
        else if (value == "false") {
            ctx.useOpBook = false;
        }
        else {
            std::cerr << "Invalid value '" << value << "'. Expected 'true' or 'false'." << std::endl;
        }
    }
}

static void cmdSetoption(UCIContext& ctx, const CommandArgs& args) {
    if (ctx.state != IDLE) {
        std::cerr << "Can only change option when Luna is not busy." << std::endl;
        return;
    }
    // Luna not busy, allow option to set
    // Parse parameters
    for (int i = 0; i < args.size(); ++i) {
        if (args[i] != "name") {
            // Unexpected
            continue;
        }

        // args[i] == "name"
        i++;
        if (i >= args.size()) {
            // Unexpected end of option
            break;
        }

        std::string optName = std::string(args[i++]);

        if (i >= args.size() || args[i] != "value") {
            // Option ended
            i--;
            // Parameterless option
            processOption(ctx, optName, "");
            continue;
        }

        // args[i] == "value"
        i++;
        if (i >= args.size()) {
            // Unexpected end of option
            break;
        }
        processOption(ctx, optName, args[i]);
    }

}

static void cmdUcinewgame(UCIContext& ctx, const CommandArgs& args) {
}

static void playMovesAfterPos(UCIContext& ctx,
                              CommandArgs::const_iterator begin,
                              CommandArgs::const_iterator end) {
    if (begin == end) {
        std::cerr << "Expected at least one move to be played.\n";
        return;
    }

    MoveList legalMoves;

    for (auto it = begin; it != end; ++it) {
        Move move = Move(ctx.pos, *it);

        if (move == MOVE_INVALID) {
            std::cerr << "Invalid move '" << *it << "'." << std::endl;
            break;
        }

        ctx.pos.makeMove(move);
        legalMoves.clear();
    }
}

static void cmdPosition(UCIContext& ctx, const CommandArgs& args) {
    if (args[0] == "startpos") {
        ctx.pos = Position::getInitialPosition();

        if (args.size() > 1) {
            // User has maybe provided moves to be played.
            if (args[1] != "moves") {
                errorWrongArg("position", args[1]);
                return;
            }

            // They indeed provided them, play them.
            playMovesAfterPos(ctx, args.begin() + 2, args.end());
        }
        return;
    }

    if (args[0] != "fen") {
        errorWrongArg("position", args[0]);
        return;
    }

    int movesArgsIdx = -1;

    // FEN argument given, process it
    std::stringstream fenStream;
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == "moves") {
            // From beyond here, the fen string has ended and the user
            // has provided moves to be played on the position.
            movesArgsIdx = i;
            break;
        }

        fenStream << args[i] << ' ';
    }
    std::string fen = fenStream.str();

    // A FEN string was provided
    auto posOpt = Position::fromFen(fen);
    if (!posOpt.has_value()) {
        std::cerr << "Provided FEN string '" << fen << "' is invalid." << std::endl;
        return;
    }

    // FEN string successfully interpreted, set the position accordingly
    ctx.pos = *posOpt;

    if (movesArgsIdx == -1) {
        return;
    }
    // User requested some moves to be played after the position, play them.
    movesArgsIdx++;
    playMovesAfterPos(ctx, args.cbegin() + movesArgsIdx, args.cend());
}

static bool readTime(std::string_view sv, int& dest) {
    bool success = strutils::tryParseInteger(sv, dest);
    if (!success) {
        // Invalid depth value
        std::cerr << "Unexpected time value '" << sv << "'." << std::endl;
        return false;
    }
    return true;
}

static void goSearch(UCIContext& ctx, const Position& pos, ai::SearchSettings& searchSettings) {
    if (ctx.useOpBook) {
        // Use opening book if position is covered in it
        const auto& book = OpeningBook::getDefault();
        Move move = book.getRandomMoveForPosition(pos);
        if (move != MOVE_INVALID) {
            // We found a book move
            std::cout << "bestmove " << move << std::endl;
            ctx.state = IDLE;
            return;
        }
    }
    ctx.searcher = ai::AlphaBetaSearcher(ctx.hce);

    TimePoint startTime = Clock::now();
    searchSettings.onPvFinish = [startTime](ai::SearchResults res, int pv) {
        ai::SearchedVariation& var = res.searchedVariations[pv];

        std::cout << "info depth " << res.searchedDepth;

        std::cout << " multipv " << pv + 1;

        // Print score
        if (std::abs(var.score) < ai::FORCED_MATE_THRESHOLD) {
            std::cout << " score cp " << var.score / 10;
        }
        else {
            // Forced checkmate found
            int mateScore = var.score > 0 ? ai::MATE_SCORE : -ai::MATE_SCORE;
            int pliesToMate = mateScore - var.score + 1;
            std::cout << " score mate " << (pliesToMate + 1) / 2;
        }

        // Is it lowerbound, upperbound, or exact (do nothing)?
        if (var.type == ai::TranspositionTable::LOWERBOUND) {
            std::cout << " lowerbound";
        }
        else if (var.type == ai::TranspositionTable::UPPERBOUND) {
            std::cout << " upperbound";
        }

        // Show the line itself
        std::cout << " pv";
        for (Move m: var.moves) {
            std::cout << " " << m;
        }

        std::cout << " nodes " << res.visitedNodes;
        std::cout << " nps " << res.getNPS();

        std::cout << " time " << deltaMs(Clock::now(), startTime);

        std::cout << std::endl;
    };

    // Spawn a secondary thread to run the search in order to keep listening
    // for user input.
    // The solution below is probably flawed due to the fact that we are not
    // able to properly handle exceptions thrown from the thread
    std::thread([&ctx, searchSettings, pos]() {
        try {
            ai::SearchResults res = ctx.searcher.search(pos, searchSettings);
            std::cout << "bestmove " << res.bestMove << std::endl;

            ctx.state = IDLE;
        }
        catch (const std::exception& e) {
            std::cerr << "Unhandled exception during search: " << std::endl
                << e.what() << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }).detach();
}

static void cmdGo(UCIContext& ctx, const CommandArgs& args) {
    if (ctx.state != IDLE) {
        std::cerr << "Cannot call go while a search is currently running. Call 'stop' first." << std::endl;
        return;
    }
    // Create position clone
    Position pos = ctx.pos;

    ctx.state = BUSY;

    TimeControl timeControl[CL_COUNT];

    MoveList searchMoves;

    ai::SearchSettings searchSettings;

    // Parse arguments
    for (int i = 0; i < args.size(); ++i) {
        const auto arg = args[i];

        if (arg == "searchmoves") {
            // User wants to search only some specific moves
            i++;
            for (Move move = Move(pos, args[i]); move != MOVE_INVALID && i < args.size(); i++) {
                searchMoves.add(move);
            }
        }
        else if (arg == "depth") {
            // User wants to limit the search depth to a specific value
            int depth;
            bool succ = strutils::tryParseInteger(args[++i], depth);
            if (succ && depth >= 1) {
                searchSettings.maxDepth = depth;
            }
            else {
                // Invalid depth value
                std::cerr << "Unexpected depth value '" << args[i] << "'." << std::endl;
            }
        }
        else if (arg == "wtime") {
            // Defines white color base time
            readTime(args[++i], timeControl[CL_WHITE].time);
            timeControl[CL_WHITE].mode = TC_TOURNAMENT;
        }
        else if (arg == "winc") {
            // Defines white color time increment
            readTime(args[++i], timeControl[CL_WHITE].increment);
            timeControl[CL_WHITE].mode = TC_TOURNAMENT;
        }
        else if (arg == "btime") {
            // Defines black color base time
            readTime(args[++i], timeControl[CL_BLACK].time);
            timeControl[CL_BLACK].mode = TC_TOURNAMENT;
        }
        else if (arg == "binc") {
            // Defines black color time increment
            readTime(args[++i], timeControl[CL_BLACK].increment);
            timeControl[CL_BLACK].mode = TC_TOURNAMENT;
        }
        else if (arg == "movetime") {
            // Defines the move time, a time in milliseconds for each move.
            readTime(args[++i], timeControl[pos.getColorToMove()].time);
            timeControl[pos.getColorToMove()].mode = TC_MOVETIME;
        }
        else if (arg == "infinite") {
            timeControl[pos.getColorToMove()].mode = TC_INFINITE;
        }
    }

    // Check if searchmoves option was used
    if (searchMoves.size() == 0) {
        searchSettings.moveFilter = nullptr;
    }
    else {
        // Use the move search list as a filter for the moves to be searched.
        searchSettings.moveFilter = [searchMoves](Move m) {
            return searchMoves.contains(m);
        };
    }

    searchSettings.ourTimeControl = timeControl[pos.getColorToMove()];
    searchSettings.theirTimeControl = timeControl[getOppositeColor(pos.getColorToMove())];
    searchSettings.multiPvCount = ctx.multiPvCount;

    goSearch(ctx, pos, searchSettings);
}

static void cmdLunaPerft(UCIContext& ctx, const CommandArgs& args) {
    int depth;
    if (!strutils::tryParseInteger(args[0], depth)) {
        errorWrongArg("perft", args[0]);
        return;
    }

    bool pseudoLegal = false;
    bool algNotation = false;
    for (auto it = args.begin() + 1; it != args.end(); ++it) {
        auto arg = *it;

        if (arg == "--pseudo") {
            pseudoLegal = true;
        }
        else if (arg == "--alg") {
            algNotation = true;
        }
    }

    auto before = Clock::now();

    ui64 res = perft(ctx.pos, depth, true, pseudoLegal, algNotation);

    i64 elapsed = deltaMs(Clock::now(), before);

    std::cout << "Nodes: " << res << std::endl;
    std::cout << "Time: " << elapsed << "ms" << std::endl;
    std::cout << "NPS: " << ui64(double(res) / double(elapsed + 1) * 1000) << std::endl;
}

static void cmdDoMoves(UCIContext& ctx, const CommandArgs& args) {
    for (const auto& arg: args) {
        Move move(ctx.pos, arg);

        if (move == MOVE_INVALID) {
            return;
        }

        if (!ctx.pos.isMoveLegal(move)) {
            std::cerr << "Illegal move " << move << "." << std::endl;
            break;
        }

        ctx.pos.makeMove(move);
    }
}

static void cmdTakeback(UCIContext& ctx, const CommandArgs& args) {
    int n;

    if (!args.empty()) {
        n = strutils::tryParseInteger(args[0], n) ? n : 1;
    }
    else {
        n = 1;
    }

    for (int i = 0; i < n; ++i) {
        ctx.pos.undoMove();
    }

    std::cout << ctx.pos << std::endl;
}

static void stopSearch(UCIContext& ctx) {
    ctx.searcher.stop();
}

static void cmdStop(UCIContext& ctx, const CommandArgs& args) {
    if (ctx.state != BUSY) {
        // No searches ongoing
        std::cerr << "Not searching at the moment." << std::endl;
        return;
    }

    stopSearch(ctx);
}

static void cmdGetpos(UCIContext& ctx, const CommandArgs& args) {
    std::cout << ctx.pos << std::endl;
}

static void cmdGetfen(UCIContext& ctx, const CommandArgs& args) {
    std::cout << ctx.pos.toFen() << std::endl;
}

static int doEval(UCIContext& ctx, int depth) {
    int eval = 0;
    if (depth == 0) {
        auto& evaluator = ctx.searcher.getEvaluator();
        evaluator.setPosition(ctx.pos);
        eval = evaluator.evaluate();
    }
    else {
        ai::SearchSettings settings;
        settings.maxDepth = depth;
        settings.ourTimeControl.mode = TC_INFINITE;
        settings.theirTimeControl.mode = TC_INFINITE;
        eval = ctx.searcher.search(ctx.pos, settings).bestScore;
    }

    if (ctx.pos.getColorToMove() == CL_BLACK) {
        eval *= -1;
    }
    return eval;
}

static void cmdEval(UCIContext& ctx, const CommandArgs& args) {
    if (args.size() > 2) {
        std::cerr << "Too many arguments for eval." << std::endl;
        return;
    }
    int depth = 0;
    if (args.size() == 1) {
        if (!strutils::tryParseInteger(args[0], depth)) {
            errorWrongArg("eval", args[0]);
            return;
        }
    }

    PieceSquareTable pst;
    Position& pos = ctx.pos;
    Bitboard pieces = pos.getCompositeBitboard();

    int currentEval = doEval(ctx, depth);

    for (Square s: pieces) {
        Piece p = pos.getPieceAt(s);
        if (p.getType() == PT_KING) {
            continue;
        }

        pos.setPieceAt(s, PIECE_NONE);
        int evalWithoutPiece = doEval(ctx, depth);
        pos.setPieceAt(s, p);

        int delta = currentEval - evalWithoutPiece;

        pst.valueAt(s, CL_WHITE) = delta;
    }

    std::cout << pst << std::endl;
    std::cout << "Total evaluation: "
        << std::setprecision(2)
        << double(currentEval) / 1000
        << std::endl;
}

static void cmdTune(UCIContext& ctx, const CommandArgs& args) {
    namespace fs = std::filesystem;

    fs::path positionsPath = "positions.csv";

    for (int i = 0; i < args.size(); ++i) {
        const auto arg = args[i];

        if (arg == "file") {
            i++;
            if (i >= args.size()) {
                std::cerr << "Expected a path." << std::endl;
                return;
            }
            positionsPath = args[i];
        }
    }

    std::vector<ai::TuningSamplePosition> samplePositions;
    try {
        std::ifstream ifstream(positionsPath);
        ifstream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
        samplePositions = ai::fetchSamplePositionsFromCSV(ifstream);

        ifstream.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Error when reading sample positions: " << e.what() << std::endl;
        return;
    }

    auto eval = ctx.hce;
    auto& tbl = eval->getWeights();

    ai::TuningSettings settings;

    ai::tune(tbl, samplePositions, settings);
}

static void cmdLoadweights(UCIContext& ctx, const CommandArgs& args) {
    namespace fs = std::filesystem;

    fs::path path = args[0];
    try {
        nlohmann::json weightsJson = nlohmann::json::parse(utils::readFromFile(path));


        ai::HCEWeightTable* weights = new ai::HCEWeightTable(weightsJson);
        ctx.hce->setWeights(weights);

        if (ctx.hceWeights != ai::getDefaultHCEWeights()) {
            delete ctx.hceWeights;
        }
        ctx.hceWeights = weights;

        std::cout << "Succesfully loaded weights from " << fs::absolute(path) << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load weights from " << fs::absolute(path) << ":\n" << e.what() << std::endl;
    }
}

static void cmdSaveweights(UCIContext& ctx, const CommandArgs& args) {
    namespace fs = std::filesystem;

    fs::path path = args[0];
    try {
        nlohmann::json weightsJson = ctx.hce->getWeights();
        utils::writeToFile(path, weightsJson.dump(2));
        std::cout << "Succesfully saved weights to " << fs::absolute(path) << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to saved weights to " << fs::absolute(path) << ":\n" << e.what() << std::endl;
    }
}

#ifndef NDEBUG
static void cmdAttacks(UCIContext& ctx, const CommandArgs& args) {
    Color c = ctx.pos.getColorToMove();
    PieceType pt = PT_NONE;

    for (const auto& argView: args) {
        std::string arg = std::string(argView);
        if (arg[arg.size() - 1] == 's') {
            arg.pop_back();
        }

        strutils::toLower(arg);

        if (arg == "pawn") {
            pt = PT_PAWN;
        }
        else if (arg == "knight") {
            pt = PT_KNIGHT;
        }
        else if (arg == "bishop") {
            pt = PT_BISHOP;
        }
        else if (arg == "rook") {
            pt = PT_ROOK;
        }
        else if (arg == "queen") {
            pt = PT_QUEEN;
        }
        else if (arg == "king") {
            pt = PT_KING;
        }
        else if (arg == "white") {
            c = CL_WHITE;
        }
        else if (arg == "black") {
            c = CL_BLACK;
        }
        else {
            errorWrongArg("attacks", arg);
        }
    }

    std::cout << ctx.pos.getAttacks(c, pt) << std::endl;
}

static void cmdPins(UCIContext& ctx, const CommandArgs& args) {
    Bitboard pinned = ctx.pos.getPinned();
    std::cout << "Pinned pieces:\n" << pinned << std::endl;

    for (auto s : pinned) {
        Piece pinnedPiece = ctx.pos.getPieceAt(s);
        Square pinnerSqr = ctx.pos.getPinner(s);
        Piece pinnerPiece = ctx.pos.getPieceAt(pinnerSqr);

        std::cout << getPieceTypeName(pinnedPiece.getType()) << " on " << getSquareName(s)
                  << " is pinned by a " << getPieceTypeName(pinnerPiece.getType()) << " on " << getSquareName(pinnerSqr)
                  << std::endl;
    }
}

static void cmdBetween(UCIContext& ctx, const CommandArgs& args) {
    Square a = getSquare(args[0]);
    Square b = getSquare(args[1]);

    std::cout << "Between " << getSquareName(a) << " and " << getSquareName(b) << ":" << std::endl;
    std::cout << bbs::getSquaresBetween(a, b) << std::endl;
}
#endif


static std::unordered_map<std::string, Command> generateCommands() {
    std::unordered_map<std::string, Command> cmds;

    // UCI commands:
    cmds["uci"] = Command(cmdUci, 0);
    cmds["quit"] = Command(cmdQuit, 0);
    cmds["debug"] = Command(cmdDebug, 1);
    cmds["isready"] = Command(cmdIsready, 0);
    cmds["setoption"] = Command(cmdSetoption, 1, false);
    cmds["ucinewgame"] = Command(cmdUcinewgame, 0);
    cmds["position"] = Command(cmdPosition, 1, false);
    cmds["go"] = Command(cmdGo, 0, false);
    cmds["stop"] = Command(cmdStop, 0);

    // Luna commands:
    cmds["domoves"] = Command(cmdDoMoves, 1, false);
    cmds["getfen"] = Command(cmdGetfen, 0);
    cmds["getpos"] = Command(cmdGetpos, 0);
    cmds["perft"] = Command(cmdLunaPerft, 1, false);
    cmds["takeback"] = Command(cmdTakeback, 0, false);
    cmds["eval"] = Command(cmdEval, 0, false);
//    cmds["tune"] = Command(cmdTune, 0, false);
    cmds["saveweights"] = Command(cmdSaveweights, 1);
    cmds["loadweights"] = Command(cmdLoadweights, 1);

#ifndef NDEBUG
    // Debug commands
    cmds["db_between"] = Command(cmdBetween, 2);
    cmds["db_pins"] = Command(cmdPins, 0);
    cmds["db_attacks"] = Command(cmdAttacks, 0, false);
#endif

    return cmds;
}

//
//  UCI Main loop functions:
//

static void handleInput(UCIContext& ctx, std::unordered_map<std::string, Command>& cmds) {
    std::string in;
    std::getline(std::cin, in);

    // Generate arguments
    CommandArgs args;

    // Process input
    strutils::reduceWhitespace(in);
    strutils::split(in, args, " ");

    if (!args.empty()) {
        std::string cmd = std::string(*args.begin());
        // First member of the list is the command itself, remove it.
        args.erase(args.begin());

        // Find function that matches the command
        auto it = cmds.find(cmd);
        if (it == cmds.end()) {
            // Unknown command
            std::cerr << "Unknown command '" << cmd << "'." << std::endl;
        }
        else {
            // Command found, execute it.
            Command& c = it->second;
            // First check if there's an argument count mismatch.
            // If not, execute the command.
            if (args.size() < size_t(c.minExpectedArgs)) {
                std::cerr << "Expected at least " << c.minExpectedArgs << " argument(s) for '" << cmd
                          << "', got " << args.size() << "." << std::endl;
            }
            else if (args.size() > size_t(c.minExpectedArgs) && c.exactArgsCount) {
                std::cerr << "Expected only " << c.minExpectedArgs << " argument(s) for '" << cmd
                          << "', got " << args.size() << "." << std::endl;
            }
            else {
                it->second.function(ctx, args);
            }
        }
    }
}

static void inputThreadMain(UCIContext& ctx) {
    // Generate commands dictionary
    auto dict = generateCommands();

    while (ctx.state != STOPPING) {
        handleInput(ctx, dict);
    }
}

int uciMain() {
    std::shared_ptr<UCIContext> ctx = std::make_shared<UCIContext>();

    inputThreadMain(*ctx);

    return 0;
}

}