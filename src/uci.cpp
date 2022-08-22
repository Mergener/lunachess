#include "uci.h"

#include <cstdlib>
#include <fstream>
#include <future>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "perft.h"
#include "strutils.h"
#include "lock.h"
#include "position.h"
#include "clock.h"

#include "ai/search.h"
#include "ai/neural/nn.h"

namespace lunachess {

using CommandArgs = std::vector<std::string_view>;

enum UCIState {
    WAITING,
    WORKING,
    STOPPING
};

struct UCIContext {

    // Chess state
    Position pos = Position::getInitialPosition();
    std::vector<Move> moveHistory;

    // UCI settings
    bool debugMode = false;
    int multiPvCount = 1;

    // Internal state
    UCIState state = WAITING;

    std::queue<std::function<void()>> workQueue;
    Lock workQueueLock;

    // Search settings
    ai::AlphaBetaSearcher searcher;

    ai::neural::NeuralEvaluator neural;
};

static void schedule(UCIContext& ctx, std::function<void()> work) {
    ctx.workQueueLock.lock();
    ctx.workQueue.push(work);
    ctx.workQueueLock.unlock();
}

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

    if (defaultVal != "") {
        std::cout << " default " << defaultVal;
    }
    if (minVal != "") {
        std::cout << " min " << minVal;
    }
    if (maxVal != "") {
        std::cout << " max " << maxVal;
    }

    std::cout << std::endl;
}

static void cmdUci(UCIContext& ctx, const CommandArgs& args) {
    std::cout << "id name LunaChess" << std::endl;
    std::cout << "id author Thomas Mergener" << std::endl;
    displayOption(ctx, "MultiPV", "spin", "1", "1", "500");
    displayOption(ctx, "Hash", "spin", strutils::toString(ai::TranspositionTable::DEFAULT_SIZE_MB), "1", "1048576");
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
}

static void cmdSetoption(UCIContext& ctx, const CommandArgs& args) {
    if (ctx.state != WAITING) {
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
            processOption(ctx, optName, nullptr);
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

static void cmdRegister(UCIContext& ctx, const CommandArgs& args) {
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
        ctx.moveHistory.push_back(move);
        legalMoves.clear();
    }
}

static void cmdPosition(UCIContext& ctx, const CommandArgs& args) {
    if (args[0] == "startpos") {
        ctx.moveHistory.clear();
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
        std::cout << ctx.pos << std::endl;
        return;
    }

    if (args[0] != "fen") {
        errorWrongArg("position", args[0]);
        return;
    }

    ctx.moveHistory.clear();
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

    // FEN string succesfully interpreted, set the position accordingly
    ctx.pos = std::move(*posOpt);

    if (movesArgsIdx == -1) {
        return;
    }
    // User requested some moves to be played after the position, play them.
    movesArgsIdx++;
    playMovesAfterPos(ctx, args.cbegin() + movesArgsIdx, args.cend());
}

static bool readTime(std::string_view sv, int& dest) {
    bool succ = strutils::tryParseInteger(sv, dest);
    if (!succ) {
        // Invalid depth value
        std::cerr << "Unexpected time value '" << sv << "'." << std::endl;
        return false;
    }
    return true;
}

static void goSimulate(UCIContext& ctx, ai::SearchSettings& searchSettings) {
    constexpr TimeControlMode FALLBACK_TC_MODE = TC_FISCHER;
    constexpr ui64 FALLBACK_TC_TIME = 180000;
    constexpr ui64 FALLBACK_TC_INC = 2000;

    TimeControl wtc;
    TimeControl btc;

    if (ctx.pos.getColorToMove() == CL_WHITE) {
        wtc = searchSettings.ourTimeControl;
        btc = searchSettings.theirTimeControl;
    }
    else {
        wtc = searchSettings.theirTimeControl;
        btc = searchSettings.ourTimeControl;
    }

    if (wtc.mode == TC_INFINITE &&
        searchSettings.maxDepth >= ai::MAX_SEARCH_DEPTH) {
        std::cout << "White time control is set to 'infinite' mode without depth limit.";
        std::cout << " Their time control is being altered to 3+2 blitz.";
        std::cout << std::endl;
        wtc.mode = FALLBACK_TC_MODE;
        wtc.time = FALLBACK_TC_TIME;
        wtc.increment = FALLBACK_TC_INC;
    }
    if (btc.mode == TC_INFINITE &&
        searchSettings.maxDepth >= ai::MAX_SEARCH_DEPTH) {
        std::cout << "Black time control is set to 'infinite' mode without depth limit.";
        std::cout << " Their time control is being altered to 3+2 blitz.";
        std::cout << std::endl;
        btc.mode = FALLBACK_TC_MODE;
        btc.time = FALLBACK_TC_TIME;
        btc.increment = FALLBACK_TC_INC;
    }

    if (ctx.pos.getColorToMove() == CL_WHITE) {
        searchSettings.ourTimeControl = wtc;
        searchSettings.theirTimeControl = btc;
    }
    else {
        searchSettings.ourTimeControl = btc;
        searchSettings.theirTimeControl = wtc;
    }

    schedule(ctx, [=, &ctx] {
        std::vector<Move> gameHistory;
        ai::SearchSettings settings = searchSettings;
        ChessResult res = RES_UNFINISHED;

        Position pos = ctx.pos;

        try {
            while (res == RES_UNFINISHED) {
                ai::SearchResults sres = ctx.searcher.search(pos, settings);
                if (ctx.state == STOPPING) {
                    break;
                }

                pos.makeMove(sres.bestMove);
                gameHistory.push_back(sres.bestMove);
                if (settings.ourTimeControl.mode == TC_FISCHER) {
                    settings.ourTimeControl.time -= sres.getSearchTime();
                    settings.ourTimeControl.time += settings.ourTimeControl.increment;
                }

                std::cout << "move " << sres.bestMove
                          << " score cp " << sres.bestScore / 10
                          << " depth " << sres.searchedDepth
                          << " time " << sres.getSearchTime()
                          << std::endl;

                bool hasTime = settings.ourTimeControl.mode == TC_INFINITE
                        || settings.ourTimeControl.time > 0;

                res = pos.getResult(CL_WHITE, hasTime);

                std::swap(settings.ourTimeControl, settings.theirTimeControl);
            }
        }
        catch (const std::exception& ex) {
            ctx.state = WAITING;
            std::cerr << "Game aborted, error";
            throw ex;
        }

        switch (res) {
            case RES_UNFINISHED:
                std::cout << "Game aborted." << std::endl;
                break;

            case RES_DRAW_STALEMATE:
                std::cout << "Draw by stalemate." << std::endl;
                break;

            case RES_DRAW_REPETITION:
                std::cout << "Draw by repetition." << std::endl;
                break;

            case RES_DRAW_TIME_NOMAT:
                std::cout << "Draw by insufficient material and timeout." << std::endl;
                break;

            case RES_DRAW_NOMAT:
                std::cout << "Draw by insufficient material." << std::endl;
                break;

            case RES_DRAW_RULE50:
                std::cout << "Draw by 50 move rule." << std::endl;
                break;

            case RES_WIN_CHECKMATE:
                std::cout << "White wins by checkmate." << std::endl;
                break;

            case RES_WIN_TIME:
                std::cout << "White wins on time." << std::endl;
                break;

            case RES_WIN_RESIGN:
                std::cout << "White wins by resignation." << std::endl;
                break;

            case RES_LOSS_CHECKMATE:
                std::cout << "Black wins by checkmate." << std::endl;
                break;

            case RES_LOSS_TIME:
                std::cout << "Black wins on time." << std::endl;
                break;

            case RES_LOSS_RESIGN:
                std::cout << "Black wins by resignation." << std::endl;
                break;
        }

        ctx.state = WAITING;
    });
}

static void goSearch(UCIContext& ctx, const Position& pos, ai::SearchSettings& searchSettings) {

    TimePoint startTime = Clock::now();
    searchSettings.onPvFinish = [startTime](ai::SearchResults res, int pv) {
        ai::SearchedVariation& var = res.searchedVariations[pv];

        std::cout << "info depth " << res.searchedDepth;

        std::cout << " multipv " << pv + 1;

        // Print score
        if (std::abs(var.score) < ai::FORCED_MATE_THRESHOLD) {
            std::cout << " score cp " << var.score / 10;
        } else {
            // Forced checkmate found
            int mateScore = var.score > 0 ? ai::MATE_SCORE : -ai::MATE_SCORE;
            int pliesToMate = mateScore - var.score + 1;
            std::cout << " score mate " << pliesToMate / 2;
        }

        // Is it lowerbound, upperbound, or exact (do nothing)?
        if (var.type == ai::TranspositionTable::LOWERBOUND) {
            std::cout << " lowerbound";
        } else if (var.type == ai::TranspositionTable::UPPERBOUND) {
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

    //ctx.searcher.getTT().clear();

    schedule(ctx, [=, &ctx] {
        ai::SearchResults res = ctx.searcher.search(pos, searchSettings);
        std::cout << "bestmove " << res.bestMove << std::endl;

        ctx.state = WAITING;
    });
}

static void cmdGo(UCIContext& ctx, const CommandArgs& args) {
    if (ctx.state != WAITING) {
        std::cerr << "Cannot call go while a search is currently running. Call 'stop' first." << std::endl;
        return;
    }
    // Create position clone
    Position pos = ctx.pos;

    enum {
        SEARCH,
        SIMULATE
    } goMode = SEARCH;

    ctx.state = WORKING;

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
        } else if (arg == "depth") {
            // User wants to limit the search depth to a specific value
            int depth;
            bool succ = strutils::tryParseInteger(args[++i], depth);
            if (succ && depth >= 1) {
                searchSettings.maxDepth = depth;
            } else {
                // Invalid depth value
                std::cerr << "Unexpected depth value '" << args[i] << "'." << std::endl;
            }
        } else if (arg == "wtime") {
            // Defines white color base time
            readTime(args[++i], timeControl[CL_WHITE].time);
            timeControl[CL_WHITE].mode = TC_FISCHER;
        } else if (arg == "winc") {
            // Defines white color time increment
            readTime(args[++i], timeControl[CL_WHITE].increment);
            timeControl[CL_WHITE].mode = TC_FISCHER;
        } else if (arg == "btime") {
            // Defines black color base time
            readTime(args[++i], timeControl[CL_BLACK].time);
            timeControl[CL_BLACK].mode = TC_FISCHER;
        } else if (arg == "binc") {
            // Defines black color time increment
            readTime(args[++i], timeControl[CL_BLACK].increment);
            timeControl[CL_BLACK].mode = TC_FISCHER;
        } else if (arg == "movetime") {
            // Defines the move time, a time in milliseconds for each move.
            readTime(args[++i], timeControl[pos.getColorToMove()].time);
            timeControl[pos.getColorToMove()].mode = TC_MOVETIME;
        } else if (arg == "infinite") {
            timeControl[pos.getColorToMove()].mode = TC_INFINITE;
        }
        else if (arg == "simulate") {
            goMode = SIMULATE;
        }
    }

    // Check if searchmoves option was used
    if (searchMoves.size() == 0) {
        searchSettings.moveFilter = nullptr;
    } else {
        // Use the move search list as a filter for the moves to be searched.
        searchSettings.moveFilter = [searchMoves](Move m) {
            return searchMoves.contains(m);
        };
    }

    searchSettings.ourTimeControl = timeControl[pos.getColorToMove()];
    searchSettings.theirTimeControl = timeControl[getOppositeColor(pos.getColorToMove())];
    searchSettings.multiPvCount = ctx.multiPvCount;

    switch (goMode) {
        case SEARCH:
            goSearch(ctx, pos, searchSettings);
            break;

        case SIMULATE:
            goSimulate(ctx, searchSettings);
            break;
    }
}

static void cmdLunaPerft(UCIContext& ctx, const CommandArgs& args) {
    int depth;
    if (!strutils::tryParseInteger(args[0], depth)) {
        errorWrongArg("perft", args[0]);
        return;
    }

    bool pseudoLegal = false;
    for (auto it = args.begin() + 1; it != args.end(); ++it) {
        auto arg = *it;

        if (arg == "pseudo") {
            pseudoLegal = true;
        }
    }

    Clock clock;
    auto before = clock.now();

    ui64 res = perft(ctx.pos, depth, pseudoLegal);

    i64 elapsed = deltaMs(clock.now(), before);

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
        ctx.moveHistory.push_back(move);
    }
    std::cout << ctx.pos << std::endl;
}

static void cmdTakeback(UCIContext& ctx, const CommandArgs& args) {
    int n;

    if (args.size() >= 1) {
        n = strutils::tryParseInteger(args[0], n) ? n : 1;
    }
    else {
        n = 1;
    }

    for (int i = 0; i < n; ++i) {
        ctx.pos.undoMove();
        ctx.moveHistory.pop_back();
    }

    std::cout << ctx.pos << std::endl;
}

static void stopSearch(UCIContext& ctx) {
    ctx.searcher.stop();
}

static void cmdStop(UCIContext& ctx, const CommandArgs& args) {
    if (ctx.state != WORKING) {
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

static void cmdMovehist(UCIContext& ctx, const CommandArgs& args) {
    for (Move m: ctx.moveHistory) {
        std::cout << m << " ";
    }
    std::cout << std::endl;
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

static void cmdNeural(UCIContext& ctx, const CommandArgs& args) {
    std::cout << ctx.neural.evaluate(ctx.pos) << std::endl;
}

static std::unordered_map<std::string, Command> generateCommands() {
    std::unordered_map<std::string, Command> cmds;

    // UCI commands:
    cmds["uci"] = Command(cmdUci, 0);
    cmds["quit"] = Command(cmdQuit, 0);
    cmds["debug"] = Command(cmdDebug, 1);
    cmds["isready"] = Command(cmdIsready, 0);
    cmds["setoption"] = Command(cmdSetoption, 1, false);
    cmds["register"] = Command(cmdRegister, 1, false);
    cmds["ucinewgame"] = Command(cmdUcinewgame, 0);
    cmds["position"] = Command(cmdPosition, 1, false);
    cmds["go"] = Command(cmdGo, 0, false);
    cmds["stop"] = Command(cmdStop, 0);

    // Luna commands:
    cmds["perft"] = Command(cmdLunaPerft, 1, false);
    cmds["domoves"] = Command(cmdDoMoves, 1, false);
    cmds["takeback"] = Command(cmdTakeback, 0, false);
    cmds["getpos"] = Command(cmdGetpos, 0);
    cmds["getfen"] = Command(cmdGetfen, 0);
    cmds["movehist"] = Command(cmdMovehist, 0);
    cmds["neural"] = Command(cmdNeural, 0);

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

    if (args.size() > 0) {
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
            try {
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
            catch (const std::exception& ex) {
                std::cerr << ex.what() << std::endl;
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

static void workerThreadMain(UCIContext& ctx) {
    while (ctx.state != STOPPING) {
        std::function<void()> work = nullptr;

        ctx.workQueueLock.lock();
        if (!ctx.workQueue.empty()) {
            work = ctx.workQueue.front();
            ctx.workQueue.pop();
        }
        ctx.workQueueLock.unlock();

        if (work != nullptr) {
            work();
        }
    }
}

int uciMain() {
    std::shared_ptr<UCIContext> ctx = std::make_shared<UCIContext>();

    // Main loop
    std::thread inputThread([&ctx]() { inputThreadMain(*ctx); });

    workerThreadMain(*ctx);

    return 0;
}

}