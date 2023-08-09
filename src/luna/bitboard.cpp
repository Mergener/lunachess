#include "bitboard.h"

#include <iostream>
#include <cmath>
#include <cstring>

#include "bits.h"

#include <immintrin.h>

//#define USE_INTRIN_POPCOUNT

namespace lunachess {

std::ostream& operator<<(std::ostream& stream, Bitboard b) {
    for (i64 j = 7; j >= 0; --j) {
        stream << "[ ";

        for (i64 i = 0; i < 8; ++i) {
            ui64 bit = j * 8 + i;

            if ((b & (BIT(bit)))) {
                stream << "1";
            }
            else {
                stream << ".";
            }
            stream << " ";
        }

        stream << "]\n";
    }
    return stream;
}

int Bitboard::bitboardsBitCount[256];

namespace bbs {

Bitboard g_BishopAttacks[64][512];
Bitboard g_RookAttacks[64][4096];

static Bitboard generateSliderAttacks(Square s, Direction dir, Bitboard occ) {
    Bitboard ret = 0;

    while (true) {
        BoardFile prevFile = getFile(s);
        BoardRank prevRank = getRank(s);

        s += dir;

        int fileDelta = std::abs(getFile(s) - prevFile);
        int rankDelta = std::abs(getRank(s) - prevRank);

        if ((dir == DIR_EAST || dir == DIR_WEST) &&
            rankDelta != 0) {
            // Rank moved
            break;
        }

        if ((dir == DIR_NORTHWEST || dir == DIR_NORTHEAST ||
            dir == DIR_SOUTHEAST || dir == DIR_SOUTHWEST) && // diagonal
            (fileDelta != 1 || rankDelta != 1)) {
            // Out of diagonal
            break;
        }

        if (s < 0 || s >= 64) {
            // Out of bounds
            break;
        }

        ret.add(s);

        if (occ.contains(s)) {
            break;
        }
    }

    return ret;
}

static Bitboard generateBishopAttacks(Square s, Bitboard occ) {
    Bitboard ret = 0;

    ret |= generateSliderAttacks(s, DIR_NORTHEAST, occ);
    ret |= generateSliderAttacks(s, DIR_SOUTHEAST, occ);
    ret |= generateSliderAttacks(s, DIR_SOUTHWEST, occ);
    ret |= generateSliderAttacks(s, DIR_NORTHWEST, occ);

    return ret;
}

static Bitboard generateRookAttacks(Square s, Bitboard occ) {
    Bitboard ret = 0;

    ret |= generateSliderAttacks(s, DIR_NORTH, occ);
    ret |= generateSliderAttacks(s, DIR_SOUTH, occ);
    ret |= generateSliderAttacks(s, DIR_EAST, occ);
    ret |= generateSliderAttacks(s, DIR_WEST, occ);

    return ret;
}


static Bitboard generateOccupancy(Bitboard mask, ui64 index) {
    Bitboard ret = 0;
    Square it = 0;
    Bitboard idxBB = index;

    while (mask != 0) {
        Square s = *mask.begin();

        if (idxBB.contains(it)) {
            ret.add(s);
        }

        mask &= mask - 1;
        it++;
    }

    return ret;
}

Bitboard g_Between[64][64];

static void generateBetweenBitboards() {
    g_Between[63][63] = 0;
    for (Square a = 0; a < 63; ++a) {
        g_Between[a][a] = 0;
        for (Square b = a + 1; b < 64; ++b) {
            BoardFile aFile = getFile(a);
            BoardRank aRank = getRank(a);

            BoardFile bFile = getFile(b);
            BoardRank bRank = getRank(b);

            int deltaX = bFile - aFile;
            int deltaY = bRank - aRank; 
            // Note that deltaY is guaranteed to be >=0
            // This premise allows us to not check for deltaY < 0 in
            // the conditionals below.

            Bitboard bb = 0;

            if (deltaY == 0) { // Horizontal
                for (Square s = a + DIR_EAST; s < b; s += DIR_EAST) {
                    bb.add(s);
                }
            }
            else if (deltaX == 0) { // Vertical
                for (Square s = a + DIR_NORTH; s < b; s += DIR_NORTH) {
                    bb.add(s);
                }
            }
            else if (std::abs(deltaX) == std::abs(deltaY)) {
                // Diagonal
                if (deltaX < 0) {
                    for (Square s = a + DIR_NORTHWEST; s < b; s += DIR_NORTHWEST) {
                        bb.add(s);
                    }
                }
                else { // deltaX > 0
                    for (Square s = a + DIR_NORTHEAST; s < b; s += DIR_NORTHEAST) {
                        bb.add(s);
                    }
                }
            } 

            g_Between[a][b] = bb;
            g_Between[b][a] = bb;
        }
    }
}

Bitboard g_PawnAttacks[64][2];
Bitboard g_PawnPushes[64][2];

static void generatePawnAttacks() {
    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        Direction leftCaptureDir = getPawnLeftCaptureDir(c);
        Direction rightCaptureDir = getPawnRightCaptureDir(c);

        for (Square s = 0; s < 64; ++s) {
            Bitboard sqBB = BIT(s);
            Bitboard bb = sqBB.shifted(leftCaptureDir) | sqBB.shifted(rightCaptureDir);
            g_PawnAttacks[s][c] = bb;
        }
    }
}

static void generatePawnPushes() {
    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        Direction stepDir     = getPawnStepDir(c);
        BoardRank initialRank = getPawnInitialRank(c);

        for (Square s = 0; s < 64; ++s) {
            Bitboard sqBB  = BIT(s);
            Bitboard bb    = sqBB.shifted(stepDir);
            BoardRank rank = getRank(s);
            if (rank == initialRank) {
                // Add double pushes
                bb |= bb.shifted(stepDir);
            }

            g_PawnPushes[s][c] = bb;
        }
    }
}

void generatePopCount() {
    Bitboard::bitboardsBitCount[0] = 0;
    for (int i = 0; i < 256; i++)
    {
        Bitboard::bitboardsBitCount[i] = (i & 1) + Bitboard::bitboardsBitCount[i / 2];
    }
}

static void generateSliderBitboards() {
    for (Square s = 0; s < 64; ++s) {
        // Generate bishop attacks
        int bishopShift = BISHOP_SHIFTS[s];
        ui64 bishopEntries = (C64(1) << (64 - bishopShift));

        for (ui64 i = 0; i < bishopEntries; ++i) {
            Bitboard occ = generateOccupancy(BISHOP_MASKS[s], i);
            ui64 key = (occ * BISHOP_MAGICS[s]) >> bishopShift;
            g_BishopAttacks[s][key] = generateBishopAttacks(s, occ);
        }

        // Generate rook attacks
        int rookShift = ROOK_SHIFTS[s];
        ui64 rookEntries = (C64(1) << (64 - rookShift));

        for (ui64 i = 0; i < rookEntries; ++i) {
            Bitboard occ = generateOccupancy(ROOK_MASKS[s], i);
            ui64 key = (occ * ROOK_MAGICS[s]) >> rookShift;
            g_RookAttacks[s][key] = generateRookAttacks(s, occ);
        }
    }
}

Bitboard g_FileContestantsBBs[64][CL_COUNT];
Bitboard g_PasserBlockers[64][2];
Bitboard g_NearKingSquares[64];

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
            g_FileContestantsBBs[sq][c] = bb;
        }
    }
}


Bitboard g_VertPawnShields[64][CL_COUNT];
Bitboard g_DiagPawnShields[64][CL_COUNT];

static void initializePawnShields() {
    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        for (Square s = 0; s < 64; s++) {
            Bitboard squareBB = BIT(s);

            Bitboard vert = 0;
            Bitboard diag = 0;

            vert =  squareBB.shifted(getPawnStepDir(c)) |
                    squareBB.shifted(getPawnStepDir(c)).shifted(getPawnStepDir(c));

            diag = squareBB.shifted(getPawnLeftCaptureDir(c))
                 | squareBB.shifted(getPawnRightCaptureDir(c));

            g_VertPawnShields[s][c] = vert;
            g_DiagPawnShields[s][c] = diag;
        }
    }
}

static void generatePasserBlockerBitboards() {
    std::memset(g_PasserBlockers, 0, sizeof(g_PasserBlockers));

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
                g_PasserBlockers[s][c] = bb;
                bb.add(s);
                s = getSquare(f, r);
            }
        }
    }
}

static void generateNearKingSquares() {
    std::memset(g_NearKingSquares, 0, sizeof(g_NearKingSquares));

    for (Square s = 0; s < SQ_COUNT; ++s) {
        g_NearKingSquares[s] = bbs::getKingAttacks(s) | bbs::getKnightAttacks(s);
    }
}

void initialize() {
#ifndef USE_INTRIN_POPCOUNT
    generatePopCount();
#endif

    // Movegen bitboards
    generateSliderBitboards();
    generatePawnAttacks();
    generatePawnPushes();
    generateBetweenBitboards();

    // Static analysis bitboards
    initializePawnShields();
    generateFileContestantsBitboard();
    generatePasserBlockerBitboards();
    generateNearKingSquares();
}

} // bbs

} // lunachess