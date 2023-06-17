#include "pst.h"

namespace lunachess {

std::ostream& operator<<(std::ostream& stream, const PieceSquareTable& hotmap) {
    stream << "      ";

    for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
        stream << getFileIdentifier(f) << "     ";
    }

    for (BoardRank r = RANK_8; r >= RANK_1; --r) {
        stream << std::endl;
        stream << getRankIdentifier(r) << " |";
        for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
            int valueAt = hotmap.valueAt(getSquare(f, r), CL_WHITE);
            stream << std::setw(6)
                   << std::fixed
                   << std::setprecision(2)
                   << double(valueAt) / 1000;
        }
    }

    return stream;
}

}