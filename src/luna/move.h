#ifndef LUNA_MOVE_H
#define LUNA_MOVE_H

#include <string_view>

#include "bits.h"
#include "types.h"
#include "piece.h"
#include "staticlist.h"
#include "debug.h"

#define MOVE_INVALID (Move(0))

namespace lunachess {

class Position;

enum MoveType {

    MT_NORMAL,
    MT_SIMPLE_CAPTURE,
    MT_PROMOTION_CAPTURE,
    MT_EN_PASSANT_CAPTURE,
    MT_DOUBLE_PUSH,
    MT_CASTLES_SHORT,
    MT_CASTLES_LONG,
    MT_SIMPLE_PROMOTION,
    MT_COUNT

};

using MoveTypeMask = ui64;
static constexpr MoveTypeMask MTM_CASTLES   = bits::makeMask<MT_CASTLES_SHORT, MT_CASTLES_LONG>();
static constexpr MoveTypeMask MTM_CAPTURE   = bits::makeMask<MT_SIMPLE_CAPTURE, MT_EN_PASSANT_CAPTURE, MT_PROMOTION_CAPTURE>();
static constexpr MoveTypeMask MTM_SPECIAL   = bits::makeMask<MT_DOUBLE_PUSH, MT_CASTLES_SHORT, MT_CASTLES_LONG, MT_EN_PASSANT_CAPTURE, MT_SIMPLE_PROMOTION, MT_PROMOTION_CAPTURE>();
static constexpr MoveTypeMask MTM_PROMOTION = bits::makeMask<MT_SIMPLE_PROMOTION, MT_PROMOTION_CAPTURE>();
static constexpr MoveTypeMask MTM_QUIET     = bits::makeMask<MT_NORMAL, MT_CASTLES_LONG, MT_CASTLES_SHORT, MT_DOUBLE_PUSH>();
static constexpr MoveTypeMask MTM_ALL       = BITMASK(MT_COUNT);
static constexpr MoveTypeMask MTM_NOISY     = MTM_ALL & (~MTM_QUIET);

class Move {

//
// Encoding:
//	bits 0-5: Source square
//	bits 6-11: Destination square
//  bits 12-15: Source piece
//  bits 16-19: Captured piece
//  bits 20-22: Promotion piece type
//  bits 23-31: Flags
//

public:
    inline Square getSource() const { return m_Data & BITMASK(6); }
    inline Square getDest() const { return (m_Data >> 6) & BITMASK(6); }
    inline Piece getSourcePiece() const { return Piece((m_Data >> 12) & BITMASK(4)); }
    inline Piece getDestPiece() const { return Piece((m_Data >> 16) & BITMASK(4)); }
    inline PieceType getPromotionPiece() const { return (m_Data >> 20) & BITMASK(3); }

    inline Piece getCapturedPiece() const {
        if (getType() == MT_EN_PASSANT_CAPTURE) {
            return Piece(getOppositeColor(getSourcePiece().getColor()), PT_PAWN);
        }
        return getDestPiece();
    }

    inline MoveType getType() const { return MoveType((m_Data >> 23) & BITMASK(6)); }

    inline ui32 getRaw() const { return m_Data; }

    inline bool makesProgress() const {
        return is<MTM_CAPTURE>() || getSourcePiece().getType() == PT_PAWN;
    }

    template <MoveTypeMask FLAGS>
    inline bool is() const {
        return BIT(getType()) & FLAGS;
    }

    std::string toAlgebraic(const Position& pos) const;
    static Move fromAlgebraic(const Position& pos, std::string_view m);

    inline Move(ui32 raw = 0)
        : m_Data(raw) {
    }

    Move(const Position& pos, std::string_view v);

    inline Move(Square src, Square dst, Piece srcPiece, Piece dstPiece,
                MoveType type, PieceType promPt = PT_NONE)
            : m_Data(0) {
        ASSERT_VALID_SQUARE(src);
        ASSERT_VALID_SQUARE(dst);

        m_Data |= src & BITMASK(6);
        m_Data |= (dst & BITMASK(6)) << 6;
        m_Data |= (srcPiece.getRaw() & BITMASK(4)) << 12;
        m_Data |= (dstPiece.getRaw() & BITMASK(4)) << 16;
        m_Data |= (promPt & BITMASK(3)) << 20;
        m_Data |= (type & BITMASK(6)) << 23;
    }

    Move(const Position& pos, Square src, Square dst, PieceType promotionPieceType = PT_NONE);

    inline bool operator==(Move rhs) const { return m_Data == rhs.m_Data; }
    inline bool operator!=(Move rhs) const { return m_Data != rhs.m_Data; }
    inline operator bool() const { return *this != MOVE_INVALID; }

private:
    ui32 m_Data;
};

std::ostream& operator<<(std::ostream& stream, Move m);

} // lunachess

#endif // LUNA_MOVE_H