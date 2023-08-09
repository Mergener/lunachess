#ifndef LUNA_BITBOARD_H
#define LUNA_BITBOARD_H

#include <ostream>
#include <initializer_list>
#include <immintrin.h>

#include "types.h"
#include "piece.h"

namespace lunachess {

namespace bbs {
void generatePopCount();
}

/**
    Abstraction for uint64s that can be used as bitsets for squares on a chessboard.
*/
class Bitboard {
public:
    inline constexpr operator ui64() const {
        return m_BB;
    }

    inline bool contains(Square square) const {
        return m_BB & (C64(1) << square);
    }

    inline Bitboard& operator&=(Bitboard other) {
        m_BB &= other.m_BB;
        return *this;
    }

    inline Bitboard& operator|=(Bitboard other) {
        m_BB |= other.m_BB;
        return *this;
    }

    inline Bitboard operator^=(Bitboard other) {
        m_BB ^= other.m_BB;
        return *this;
    }

#if 0
    inline constexpr Bitboard operator~() const {
        return ~m_BB;
    }


    /** Union of bbs. */
    inline constexpr Bitboard operator|(Bitboard other) const {
        return m_BB | other.m_BB;
    }

    /** Intersection of bbs. */
    inline constexpr Bitboard operator&(Bitboard other) const  {
        return m_BB & other.m_BB;
    }

    inline constexpr Bitboard operator<<(int n) const {
        return m_BB << n;
    }

    inline Bitboard& operator<<=(int n) {
        m_BB <<= n;
        return *this;
    }

    inline constexpr Bitboard operator>>(int n) const  {
        return m_BB >> n;
    }

    inline Bitboard& operator>>=(int n) {
        m_BB >>= n;
        return *this;
    }
#endif
    /** Sets the bit of the specified square to zero. */
    inline void remove(Square sq) {
        m_BB &= ~(C64(1) << sq);
    }

    /** Sets the bit of the specified square to one. */
    inline void add(Square sq) {
        m_BB |= (C64(1) << sq);
    }

    template <Direction D>
    inline Bitboard shifted() const;

    inline Bitboard shifted(Direction d) {
        d += 9;

        switch (d) {
            case 9 + DIR_EAST:      return shifted<DIR_EAST>();
            case 9 + DIR_SOUTH:     return shifted<DIR_SOUTH>();
            case 9 + DIR_NORTH:     return shifted<DIR_NORTH>();
            case 9 + DIR_WEST:      return shifted<DIR_WEST>();
            case 9 + DIR_NORTHEAST: return shifted<DIR_NORTHEAST>();
            case 9 + DIR_SOUTHEAST: return shifted<DIR_SOUTHEAST>();
            case 9 + DIR_NORTHWEST: return shifted<DIR_NORTHWEST>();
            case 9 + DIR_SOUTHWEST: return shifted<DIR_SOUTHWEST>();

            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 9:
            case 11:
            case 12:
            case 13:
            case 14:
                break;
        }
        return 0;
    }

    /** Returns the amount of squares in this bitboard. O(1) operation. */
    inline int count() const {
        return bits::popcount(m_BB);
        /*return (bitboardsBitCount[m_BB & 0xff]
                + bitboardsBitCount[(m_BB >> 8) & 0xff]
                + bitboardsBitCount[(m_BB >> 16) & 0xff]
                + bitboardsBitCount[(m_BB >> 24) & 0xff]
                + bitboardsBitCount[(m_BB >> 32) & 0xff]
                + bitboardsBitCount[(m_BB >> 40) & 0xff]
                + bitboardsBitCount[(m_BB >> 48) & 0xff]
                + bitboardsBitCount[(m_BB >> 56) & 0xff]);*/
    }

    inline constexpr Bitboard() noexcept
            : m_BB(0) { }

    inline constexpr Bitboard(ui64 i) noexcept
            : m_BB(i) { }

    inline Bitboard& operator=(ui64 i) {
        m_BB = i;
        return *this;
    }

    inline Bitboard(const std::initializer_list<Square>& squares) {
        m_BB = 0;
        for (auto sq : squares) {
            add(sq);
        }
    }

    class Iterator {
    public:
        inline Iterator(Bitboard board, Square sq)
            : m_BB(board), m_Sq(sq) {}

        inline Iterator(Bitboard board)
            : m_BB(board), m_Sq(bits::bitScanF(m_BB)) {}

        inline constexpr Iterator(const Iterator& it) noexcept
            : m_BB(it.m_BB), m_Sq(it.m_Sq) {}

        inline constexpr Iterator(Iterator&& it) noexcept
            : m_BB(it.m_BB), m_Sq(it.m_Sq) {}

        inline Iterator& operator++() {
            m_BB &= m_BB - 1;
            m_Sq = bits::bitScanF(m_BB);

            return *this;
        }

        inline constexpr bool operator==(const Iterator& it) const {
            return m_BB == it.m_BB;
        }

        inline constexpr bool operator!=(const Iterator& it) const {
            return m_BB != it.m_BB;
        }

        inline constexpr Square operator*() const {
            return m_Sq;
        }

    private:
        ui64 m_BB;
        ui8 m_Sq;
    };

    inline Iterator cbegin() const { return Iterator(*this); }
    inline Iterator cend() const { return Iterator(0); }
    inline Iterator crbegin() const { return Iterator(*this, bits::bitScanR(m_BB)); }

    inline Iterator begin() { return cbegin(); }
    inline Iterator end() { return cend(); }
    inline Iterator rbegin() { return crbegin(); }

private:
    ui64 m_BB;

    static int bitboardsBitCount[];
    friend void bbs::generatePopCount();
};

std::ostream& operator<<(std::ostream& stream, Bitboard bitboard);

namespace bbs {

inline Bitboard getSquaresBetween(Square a, Square b) {
    extern Bitboard g_Between[64][64];
    return g_Between[a][b];
}

inline constexpr Bitboard DARK_SQUARES  = C64(0xaa55aa55aa55aa55);
inline constexpr Bitboard LIGHT_SQUARES = ~DARK_SQUARES;

constexpr Bitboard KING_ATTACKS[] {

        C64(0x0000000000000302), C64(0x0000000000000705), C64(0x0000000000000e0a), C64(0x0000000000001c14),
        C64(0x0000000000003828), C64(0x0000000000007050), C64(0x000000000000e0a0), C64(0x000000000000c040),
        C64(0x0000000000030203), C64(0x0000000000070507), C64(0x00000000000e0a0e), C64(0x00000000001c141c),
        C64(0x0000000000382838), C64(0x0000000000705070), C64(0x0000000000e0a0e0), C64(0x0000000000c040c0),
        C64(0x0000000003020300), C64(0x0000000007050700), C64(0x000000000e0a0e00), C64(0x000000001c141c00),
        C64(0x0000000038283800), C64(0x0000000070507000), C64(0x00000000e0a0e000), C64(0x00000000c040c000),
        C64(0x0000000302030000), C64(0x0000000705070000), C64(0x0000000e0a0e0000), C64(0x0000001c141c0000),
        C64(0x0000003828380000), C64(0x0000007050700000), C64(0x000000e0a0e00000), C64(0x000000c040c00000),
        C64(0x0000030203000000), C64(0x0000070507000000), C64(0x00000e0a0e000000), C64(0x00001c141c000000),
        C64(0x0000382838000000), C64(0x0000705070000000), C64(0x0000e0a0e0000000), C64(0x0000c040c0000000),
        C64(0x0003020300000000), C64(0x0007050700000000), C64(0x000e0a0e00000000), C64(0x001c141c00000000),
        C64(0x0038283800000000), C64(0x0070507000000000), C64(0x00e0a0e000000000), C64(0x00c040c000000000),
        C64(0x0302030000000000), C64(0x0705070000000000), C64(0x0e0a0e0000000000), C64(0x1c141c0000000000),
        C64(0x3828380000000000), C64(0x7050700000000000), C64(0xe0a0e00000000000), C64(0xc040c00000000000),
        C64(0x0203000000000000), C64(0x0507000000000000), C64(0x0a0e000000000000), C64(0x141c000000000000),
        C64(0x2838000000000000), C64(0x5070000000000000), C64(0xa0e0000000000000), C64(0x40c0000000000000)

};

constexpr Bitboard KNIGHT_ATTACKS[] {

        C64(0x0000000000020400), C64(0x0000000000050800), C64(0x00000000000a1100), C64(0x0000000000142200),
        C64(0x0000000000284400), C64(0x0000000000508800), C64(0x0000000000a01000), C64(0x0000000000402000),
        C64(0x0000000002040004), C64(0x0000000005080008), C64(0x000000000a110011), C64(0x0000000014220022),
        C64(0x0000000028440044), C64(0x0000000050880088), C64(0x00000000a0100010), C64(0x0000000040200020),
        C64(0x0000000204000402), C64(0x0000000508000805), C64(0x0000000a1100110a), C64(0x0000001422002214),
        C64(0x0000002844004428), C64(0x0000005088008850), C64(0x000000a0100010a0), C64(0x0000004020002040),
        C64(0x0000020400040200), C64(0x0000050800080500), C64(0x00000a1100110a00), C64(0x0000142200221400),
        C64(0x0000284400442800), C64(0x0000508800885000), C64(0x0000a0100010a000), C64(0x0000402000204000),
        C64(0x0002040004020000), C64(0x0005080008050000), C64(0x000a1100110a0000), C64(0x0014220022140000),
        C64(0x0028440044280000), C64(0x0050880088500000), C64(0x00a0100010a00000), C64(0x0040200020400000),
        C64(0x0204000402000000), C64(0x0508000805000000), C64(0x0a1100110a000000), C64(0x1422002214000000),
        C64(0x2844004428000000), C64(0x5088008850000000), C64(0xa0100010a0000000), C64(0x4020002040000000),
        C64(0x0400040200000000), C64(0x0800080500000000), C64(0x1100110a00000000), C64(0x2200221400000000),
        C64(0x4400442800000000), C64(0x8800885000000000), C64(0x100010a000000000), C64(0x2000204000000000),
        C64(0x0004020000000000), C64(0x0008050000000000), C64(0x00110a0000000000), C64(0x0022140000000000),
        C64(0x0044280000000000), C64(0x0088500000000000), C64(0x0010a00000000000), C64(0x0020400000000000)

};

constexpr Bitboard BISHOP_MASKS[] {

        C64(0x0040201008040200), C64(0x0000402010080400), C64(0x0000004020100a00), C64(0x0000000040221400),
        C64(0x0000000002442800), C64(0x0000000204085000), C64(0x0000020408102000), C64(0x0002040810204000),
        C64(0x0020100804020000), C64(0x0040201008040000), C64(0x00004020100a0000), C64(0x0000004022140000),
        C64(0x0000000244280000), C64(0x0000020408500000), C64(0x0002040810200000), C64(0x0004081020400000),
        C64(0x0010080402000200), C64(0x0020100804000400), C64(0x004020100a000a00), C64(0x0000402214001400),
        C64(0x0000024428002800), C64(0x0002040850005000), C64(0x0004081020002000), C64(0x0008102040004000),
        C64(0x0008040200020400), C64(0x0010080400040800), C64(0x0020100a000a1000), C64(0x0040221400142200),
        C64(0x0002442800284400), C64(0x0004085000500800), C64(0x0008102000201000), C64(0x0010204000402000),
        C64(0x0004020002040800), C64(0x0008040004081000), C64(0x00100a000a102000), C64(0x0022140014224000),
        C64(0x0044280028440200), C64(0x0008500050080400), C64(0x0010200020100800), C64(0x0020400040201000),
        C64(0x0002000204081000), C64(0x0004000408102000), C64(0x000a000a10204000), C64(0x0014001422400000),
        C64(0x0028002844020000), C64(0x0050005008040200), C64(0x0020002010080400), C64(0x0040004020100800),
        C64(0x0000020408102000), C64(0x0000040810204000), C64(0x00000a1020400000), C64(0x0000142240000000),
        C64(0x0000284402000000), C64(0x0000500804020000), C64(0x0000201008040200), C64(0x0000402010080400),
        C64(0x0002040810204000), C64(0x0004081020400000), C64(0x000a102040000000), C64(0x0014224000000000),
        C64(0x0028440200000000), C64(0x0050080402000000), C64(0x0020100804020000), C64(0x0040201008040200)

};

constexpr Bitboard ROOK_MASKS[] {

        C64(0x000101010101017e), C64(0x000202020202027c), C64(0x000404040404047a), C64(0x0008080808080876),
        C64(0x001010101010106e), C64(0x002020202020205e), C64(0x004040404040403e), C64(0x008080808080807e),
        C64(0x0001010101017e00), C64(0x0002020202027c00), C64(0x0004040404047a00), C64(0x0008080808087600),
        C64(0x0010101010106e00), C64(0x0020202020205e00), C64(0x0040404040403e00), C64(0x0080808080807e00),
        C64(0x00010101017e0100), C64(0x00020202027c0200), C64(0x00040404047a0400), C64(0x0008080808760800),
        C64(0x00101010106e1000), C64(0x00202020205e2000), C64(0x00404040403e4000), C64(0x00808080807e8000),
        C64(0x000101017e010100), C64(0x000202027c020200), C64(0x000404047a040400), C64(0x0008080876080800),
        C64(0x001010106e101000), C64(0x002020205e202000), C64(0x004040403e404000), C64(0x008080807e808000),
        C64(0x0001017e01010100), C64(0x0002027c02020200), C64(0x0004047a04040400), C64(0x0008087608080800),
        C64(0x0010106e10101000), C64(0x0020205e20202000), C64(0x0040403e40404000), C64(0x0080807e80808000),
        C64(0x00017e0101010100), C64(0x00027c0202020200), C64(0x00047a0404040400), C64(0x0008760808080800),
        C64(0x00106e1010101000), C64(0x00205e2020202000), C64(0x00403e4040404000), C64(0x00807e8080808000),
        C64(0x007e010101010100), C64(0x007c020202020200), C64(0x007a040404040400), C64(0x0076080808080800),
        C64(0x006e101010101000), C64(0x005e202020202000), C64(0x003e404040404000), C64(0x007e808080808000),
        C64(0x7e01010101010100), C64(0x7c02020202020200), C64(0x7a04040404040400), C64(0x7608080808080800),
        C64(0x6e10101010101000), C64(0x5e20202020202000), C64(0x3e40404040404000), C64(0x7e80808080808000)

};

constexpr int BISHOP_SHIFTS[] {

        58, 59, 59, 59, 59, 59, 59, 58,
        59, 59, 59, 59, 59, 59, 59, 59,
        59, 59, 57, 57, 57, 57, 59, 59,
        59, 59, 57, 55, 55, 57, 59, 59,
        59, 59, 57, 55, 55, 57, 59, 59,
        59, 59, 57, 57, 57, 57, 59, 59,
        59, 59, 59, 59, 59, 59, 59, 59,
        58, 59, 59, 59, 59, 59, 59, 58

};

constexpr int ROOK_SHIFTS[] {

        52, 53, 53, 53, 53, 53, 53, 52,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        53, 54, 54, 54, 54, 54, 54, 53,
        52, 53, 53, 53, 53, 53, 53, 52

};

constexpr Bitboard ROOK_MAGICS[] {

    C64(0x880005021864000), C64(0x8240032000401008), C64(0x200082040120080), C64(0x100080421001000),
    C64(0x600040850202200), C64(0x1080018004000200), C64(0x2100008200044100), C64(0x2980012100034080),
    C64(0x1b02002040810200), C64(0x410401000402002), C64(0x3003803000200080), C64(0x1801001000090020),
    C64(0x442000408120020), C64(0x800200800400), C64(0xc804200010080), C64(0x810100010000a042),
    C64(0x1218001804000), C64(0x102a0a0020408100), C64(0x6410020001100), C64(0x800090020100100),
    C64(0xc301010008000411), C64(0x800a010100040008), C64(0x1080010100020004), C64(0x8040020004810074),
    C64(0x200802080004000), C64(0x1010024240002002), C64(0x2048200102040), C64(0x8121000900100022),
    C64(0x201011100080005), C64(0x2c000480800200), C64(0x4040101000200), C64(0x8042008200040061),
    C64(0x10020c011800080), C64(0x8040402000401000), C64(0x200900082802000), C64(0x11001001000822),
    C64(0x454c800800800400), C64(0x4000800400800200), C64(0x420458804000630), C64(0x909000087000272),
    C64(0x380004020004000), C64(0x110004020004013), C64(0xa48104082020021), C64(0x98048010008008),
    C64(0x20080004008080), C64(0x202004490120028), C64(0x1810288040010), C64(0x1248004091020004),
    C64(0x900e082480450200), C64(0x820008020400080), C64(0x3820110020004100), C64(0x439821000080080),
    C64(0x2000408201200), C64(0x800400020080), C64(0x8008900801020400), C64(0xc810289047040200),
    C64(0x1401024080291202), C64(0x104100208202), C64(0x800401008200101), C64(0x8a0500044210089),
    C64(0x6001510201892), C64(0x2a82001021486402), C64(0x4200a1081004), C64(0x2040080402912),

};

constexpr Bitboard BISHOP_MAGICS[] {

    C64(0x4050041800440021), C64(0x20040408445080), C64(0xa906020a000020), C64(0x4404440080610020),
    C64(0x2021091400000), C64(0x900421000000), C64(0x480210704204), C64(0x120a42110101020),
    C64(0x200290020084), C64(0x1140040400a2020c), C64(0x8000080811102000), C64(0x404208a08a2),
    C64(0x2100084840840c10), C64(0x1061110080140), C64(0x1808210022000), C64(0x8030842211042008),
    C64(0x8401020011400), C64(0x10800810011040), C64(0x1208500bb20020), C64(0x98408404008880),
    C64(0xd2000c12020000), C64(0x4200110082000), C64(0x901200040c824800), C64(0x100220c104050480),
    C64(0x200260000a200408), C64(0x210a84090020680), C64(0x800c040202002400), C64(0x80190401080208a0),
    C64(0xc03a84008280a000), C64(0x8040804100a001), C64(0x8010010808880), C64(0x2210020004a0810),
    C64(0x8041000414218), C64(0x2842015004600200), C64(0x2102008200900020), C64(0x230a008020820201),
    C64(0xc080200252008), C64(0x9032004500c21000), C64(0x120a04010a2098), C64(0x200848582010421),
    C64(0xb0021a10061440c6), C64(0x4a0d0120100810), C64(0x80010a4402101000), C64(0x8810222018000100),
    C64(0x20081010101100), C64(0x8081000200410), C64(0x50a00800a1104080), C64(0x10020441184842),
    C64(0x4811012110402000), C64(0x12088088092a40), C64(0x8120846480000), C64(0x8800062880810),
    C64(0x4010802020412010), C64(0xc10008503006200a), C64(0x144300202042711), C64(0xa103441014440),
    C64(0x20804400c44001), C64(0x100210882300208), C64(0x8220200840413), C64(0x1144800b841400),
    C64(0x4460010010202202), C64(0x1000a10410202), C64(0x1092200481020400), C64(0x40420041c002047),

};

inline constexpr Bitboard getFileBitboard(BoardFile file) {
    constexpr Bitboard FILE_BBS[FL_COUNT] {
            C64(0x101010101010101), C64(0x202020202020202),
            C64(0x404040404040404), C64(0x808080808080808),
            C64(0x1010101010101010), C64(0x2020202020202020),
            C64(0x4040404040404040), C64(0x8080808080808080),
    };
    return FILE_BBS[file];
}

inline constexpr Bitboard getRankBitboard(BoardRank rank) {
    constexpr Bitboard RANK_BBS[RANK_COUNT]{
            C64(0xff), C64(0xff00), C64(0xff0000), C64(0xff000000),
            C64(0xff00000000), C64(0xff0000000000), C64(0xff000000000000),
            C64(0xff00000000000000)
    };
    return RANK_BBS[rank];
}

inline Bitboard getKnightAttacks(Square s) {
    return KNIGHT_ATTACKS[s];
}

inline Bitboard getKingAttacks(Square s) {
    return KING_ATTACKS[s];
}

inline Bitboard getBishopAttacks(Square s, Bitboard occ) {
    extern Bitboard g_BishopAttacks[64][512];
    occ &= BISHOP_MASKS[s];
    ui64 key = (occ * BISHOP_MAGICS[s]) >> (BISHOP_SHIFTS[s]);
    return g_BishopAttacks[s][key];
}

inline Bitboard getRookAttacks(Square s, Bitboard occ) {
    extern Bitboard g_RookAttacks[64][4096];
    occ &= ROOK_MASKS[s];
    ui64 key = (occ * ROOK_MAGICS[s]) >> (ROOK_SHIFTS[s]);
    return g_RookAttacks[s][key];
}

inline Bitboard getQueenAttacks(Square s, Bitboard occ) {
    return getBishopAttacks(s, occ) | getRookAttacks(s, occ);
}

inline Bitboard getSliderAttacks(Square s, Bitboard occ, PieceType pt) {
    switch (pt) {
        case PT_BISHOP: return getBishopAttacks(s, occ);
        case PT_ROOK:   return getRookAttacks(s, occ);
        case PT_QUEEN:  return getQueenAttacks(s, occ);
        default:        return 0;
    }
}

inline Bitboard getPawnPushes(Square s, Color c) {
    extern Bitboard g_PawnPushes[64][2];
    return g_PawnPushes[s][c];
}

/**
 * Returns a bitboard containing the squares in which a pawn
 * of color 'c' can attack from a square 's'. Note that pawn attacks
 * are diagonal, and do not include pushes (vertical).
 */
inline Bitboard getPawnAttacks(Square s, Color c) {
    extern Bitboard g_PawnAttacks[64][2];
    return g_PawnAttacks[s][c];
}

inline Bitboard getPieceAttacks(Square s, Bitboard occ, Piece piece) {
    switch (piece.getType()) {
        case PT_PAWN:   return getPawnAttacks(s, piece.getColor()) & occ;
        case PT_KNIGHT: return getKnightAttacks(s);
        case PT_BISHOP: return getBishopAttacks(s, occ);
        case PT_ROOK:   return getRookAttacks(s, occ);
        case PT_QUEEN:  return getQueenAttacks(s, occ);
        case PT_KING:   return getKingAttacks(s);
        default:        return 0;
    }
}

inline Bitboard getPieceMovements(Square s, Bitboard occ, Piece piece, Square enPassantSquare = SQ_INVALID) {
    switch (piece.getType()) {
        case PT_PAWN: {
            if (enPassantSquare != SQ_INVALID) {
                occ.add(enPassantSquare);
            }

            Bitboard pushes    = 0;
            Color color        = piece.getColor();
            Direction stepDir  = getPawnStepDir(color);
            Square squareAhead = s + stepDir;

            if (!occ.contains(squareAhead)) {
                pushes.add(squareAhead);
                if (!occ.contains(squareAhead + stepDir) &&
                    getRank(s) == getPawnInitialRank(color)) {
                    pushes.add(squareAhead + stepDir);
                }
            }

            return pushes | (getPawnAttacks(s, piece.getColor()) & occ);
        }
        case PT_KNIGHT: return getKnightAttacks(s);
        case PT_BISHOP: return getBishopAttacks(s, occ);
        case PT_ROOK:   return getRookAttacks(s, occ);
        case PT_QUEEN:  return getQueenAttacks(s, occ);
        case PT_KING:   return getKingAttacks(s);
        default:        return 0;
    }
}

/**
 * The castling path for a given color and board side.
 * This bitboard represents the squares that cannot
 * be occupied by any pieces if a king wants
 * to castle on that specific side.
 *
 * @param color The color of the king.
 * @param side The side of castling.
 * @return The castling path bitboard, as described above.
 */
inline constexpr Bitboard getInnerCastlePath(Color color, Side side) {
    constexpr Bitboard CASTLE_PATHS[CL_COUNT][SIDE_COUNT] {
        { 0x60, 0xe }, // White
        { 0x6000000000000000, 0xe00000000000000 }  // Black
    };

    return CASTLE_PATHS[color][side];
}

/**
 * The bitboard of squares that cannot be attacked by opponent pieces
 * if a king wants to castle on that specific side.
 */
inline constexpr Bitboard getKingCastlePath(Color color, Side side) {
    constexpr Bitboard KING_CASTLE_PATHS[CL_COUNT][SIDE_COUNT] {
            { 0x70, 0x1c }, // White
            { 0x7000000000000000, 0x1c00000000000000 } // Black
    };

    return KING_CASTLE_PATHS[color][side];
}

/**
 * Returns a bitboard of all the squares that can contain an opposing pawn
 * which could be pushed to attack the square 's'.
 */
inline Bitboard getFileContestantsBitboard(Square s, Color c) {
    extern Bitboard g_FileContestantsBBs[64][CL_COUNT];
    return g_FileContestantsBBs[s][c];
}

inline Bitboard getDiagonalPawnShieldBitboard(Square s, Color c) {
    extern Bitboard g_DiagPawnShields[64][CL_COUNT];
    return g_DiagPawnShields[s][c];
}

inline Bitboard getVerticalPawnShieldBitboard(Square s, Color c) {
    extern Bitboard g_VertPawnShields[64][CL_COUNT];
    return g_VertPawnShields[s][c];
}

inline Bitboard getPasserBlockerBitboard(Square s, Color c) {
    extern Bitboard g_PasserBlockers[64][2];
    return g_PasserBlockers[s][c];
}

inline Bitboard getNearKingSquares(Square s) {
    extern Bitboard g_NearKingSquares[64];
    return g_NearKingSquares[s];
}

inline Bitboard getCentralSquares() {
    constexpr Bitboard centralSquares = 0x1818000000;
    return centralSquares;
}

inline constexpr Bitboard getBoardHalf(Color c) {
    constexpr Bitboard HALVES[] {
    0xffffffff, 0xffffffff00000000
    };
    return HALVES[c];
}

inline constexpr Bitboard getBoardSide(Side s) {
    constexpr Bitboard SIDES[] {
        0xf0f0f0f0f0f0f0f0,
        0xf0f0f0f0f0f0f0f
    };
    return SIDES[s];
}

void initialize();

} // bbs

template <Direction D>
inline Bitboard Bitboard::shifted() const {
    ui64 ret = m_BB;

    // Do the shift
    if constexpr (D > 0) {
        ret <<= D;
    }
    else if constexpr (D < 0) {
        ret >>= -D;
    }

    // Apply mask for horizontal directional shifts
    if constexpr (D == DIR_WEST ||
                  D == DIR_SOUTHWEST ||
                  D == DIR_NORTHWEST) {
        ret &= ~(bbs::getFileBitboard(FL_H));
    }
    else if constexpr (D == DIR_EAST ||
                       D == DIR_SOUTHEAST ||
                       D == DIR_NORTHEAST) {
        ret &= ~(bbs::getFileBitboard(FL_A));
    }

    return ret;
}

} // lunachess

#endif // LUNA_BITBOARD_H
