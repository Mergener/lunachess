#include "aibitboards.h"

#include <cstring>

namespace lunachess::ai {

static Bitboard s_FileContestantsBBs[64][CL_COUNT];
static void generateFileContestantsBitboard() {
    // The bitboards generated here are the squares that an opponent must have pawns in
    // order to contest a piece in a given square 'sq'. Those squares are always squares
    // in files adjacent to the knight's file and in ranks in which an opponent's pawn
    // could still march their way towards 'sq'.
    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        int rankDir = c == CL_WHITE ? 1 : -1;

        for (Square sq = 0; sq < 64; ++sq) {

            Bitboard bb = 0;

            BoardFile file = getFile(sq);
            for (int f = file - 1; f <= file + 1; f += 2) {
                if (f < 0 || f > 7) {
                    continue;
                }

                for (BoardRank rank = getRank(sq) + rankDir; (rank >= 0) && (rank < 8); rank += rankDir) {
                    bb.add(rank * 8 + f);
                }
            }
            s_FileContestantsBBs[sq][(int)c - 1] = bb;
        }
    }
}

Bitboard getFileContestantsBitboard(Square s, Side side) {
    return s_FileContestantsBBs[s][(int)side - 1];
}

static Bitboard s_PawnShields[64][CL_COUNT];

static void initializePawnShields() {
    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        for (Square s = 0; s < 64; s++) {
            Bitboard bb = 0;
            Bitboard squareBB = BIT(s);

            bb = squareBB.shifted(getPawnLeftCaptureDir(c))
                 | squareBB.shifted(getPawnStepDir(c))
                 | squareBB.shifted(getPawnRightCaptureDir(c));

            bb |= bb.shifted(getPawnStepDir(c));

            s_PawnShields[s][c] = bb;
        }
    }
}

Bitboard getPawnShieldBitboard(Square s, Color c) {
    return s_PawnShields[s][c];
}

static Bitboard s_PasserBlockers[64][2];

static void generatePasserBlockerBitboards() {
    std::memset(s_PasserBlockers, 0, sizeof(s_PasserBlockers));

    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        int rankStep = c == CL_WHITE ? 1 : -1;
        BoardRank promRank, initialRank;

        if (c == CL_WHITE) {
            promRank = PAWN_PROMOTION_RANK<CL_WHITE>;
            initialRank = PAWN_INITIAL_RANK<CL_WHITE>;
        }
        else {
            promRank = PAWN_PROMOTION_RANK<CL_BLACK>;
            initialRank = PAWN_INITIAL_RANK<CL_BLACK>;
        }

        for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
            Bitboard bb = 0;

            Square s = getSquare(f, promRank);
            for (BoardRank r = promRank - rankStep; r != (initialRank - rankStep * 2); r -= rankStep) {
                s_PasserBlockers[s][c] = bb;
                bb.add(s);
                s = getSquare(f, r);
            }
        }
    }
}

Bitboard getPasserBlockerBitboard(Square s, Color c) {
    return s_PasserBlockers[s][c];
}

void initializeAIBitboards() {
    initializePawnShields();
    generateFileContestantsBitboard();
    generatePasserBlockerBitboards();
}

}