#ifndef LUNA_PIECE_H
#define LUNA_PIECE_H

#include "bits.h"
#include "types.h"

#define WHITE_PAWN   (Piece(CL_WHITE, PT_PAWN))
#define WHITE_KNIGHT (Piece(CL_WHITE, PT_KNIGHT))
#define WHITE_BISHOP (Piece(CL_WHITE, PT_BISHOP))
#define WHITE_ROOK   (Piece(CL_WHITE, PT_ROOK))
#define WHITE_QUEEN  (Piece(CL_WHITE, PT_QUEEN))
#define WHITE_KING   (Piece(CL_WHITE, PT_KING))

#define BLACK_PAWN   (Piece(CL_BLACK, PT_PAWN))
#define BLACK_KNIGHT (Piece(CL_BLACK, PT_KNIGHT))
#define BLACK_BISHOP (Piece(CL_BLACK, PT_BISHOP))
#define BLACK_ROOK   (Piece(CL_BLACK, PT_ROOK))
#define BLACK_QUEEN  (Piece(CL_BLACK, PT_QUEEN))
#define BLACK_KING   (Piece(CL_BLACK, PT_KING))

#define PIECE_NONE   (Piece(0, PT_NONE))

namespace lunachess {

using PieceTypeMask = ui64;
static constexpr PieceTypeMask PTM_SLIDERS  = bits::makeMask<PT_BISHOP, PT_ROOK, PT_QUEEN>();
static constexpr PieceTypeMask PTM_DIAGONAL = bits::makeMask<PT_BISHOP, PT_QUEEN>();
static constexpr PieceTypeMask PTM_ALL = BITMASK(PT_COUNT);

class Piece {

//
// Encoding:
//	bit 0: Color
//	bits 1-3: Type
//

public:
    inline Color getColor() const { return static_cast<Color>(m_Data & BITMASK(1)); }
    inline PieceType getType() const { return static_cast<PieceType>(m_Data >> 1); }
    inline ui32 getRaw() const { return m_Data; }

    char getIdentifier() const;

    inline bool operator==(Piece other) const { return m_Data == other.m_Data; }
    inline bool operator!=(Piece other) const { return m_Data != other.m_Data; }

    inline explicit constexpr Piece(ui8 raw = 0)
        : m_Data(raw) {
    }
    inline constexpr Piece(Color color, PieceType type)
        : m_Data((type << 1) | color) {
    }

    Piece(const Piece& rhs) = default;
    Piece(Piece&& rhs) = default;
    Piece& operator=(const Piece& rhs) = default;
    ~Piece() = default;

    static Piece fromIdentifier(char c);

private:
    ui8 m_Data = 0;
};

inline std::ostream& operator<<(std::ostream& stream, Piece p) {
    stream << p.getColor() << ' ' << p.getType();
    return stream;
}

} // lunachess

#endif // LUNA_PIECE_H
