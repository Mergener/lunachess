#include "position.h"

#include "staticanalysis.h"
#include "movegen.h"

#include <sstream>


namespace lunachess {

void Position::refreshCastles() {
    if (getPieceAt(SQ_A1) != WHITE_ROOK) {
        setCastleRights(CL_WHITE, SIDE_QUEEN, false);
    }

    if (getPieceAt(SQ_H1) != WHITE_ROOK) {
        setCastleRights(CL_WHITE, SIDE_KING, false);
    }

    if (getPieceAt(SQ_A8) != BLACK_ROOK) {
        setCastleRights(CL_BLACK, SIDE_QUEEN, false);
    }

    if (getPieceAt(SQ_H8) != BLACK_ROOK) {
        setCastleRights(CL_BLACK, SIDE_KING, false);
    }
}

void Position::updateAttacks() {
    // Update attacks
    Bitboard occ = getCompositeBitboard();

    // Reset checkers count to 0
    m_Status.nCheckers = 0;

    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        for (PieceType pt = PT_NONE; pt < PT_COUNT; ++pt) {
            m_Status.attacks[pt][c] = 0;
        }

        Color them = getOppositeColor(c);

        // Get their king's square to check for checks
        Square theirKing = getKingSquare(them);

        Bitboard pawns   = getBitboard(Piece(c, PT_PAWN));
        Bitboard knights = getBitboard(Piece(c, PT_KNIGHT));
        Bitboard bishops = getBitboard(Piece(c, PT_BISHOP));
        Bitboard rooks   = getBitboard(Piece(c, PT_ROOK));
        Bitboard queens  = getBitboard(Piece(c, PT_QUEEN));
        Bitboard kings   = getBitboard(Piece(c, PT_KING));

        for (auto s : pawns) {
            Bitboard attacks = bbs::getPawnAttacks(s, c);
            m_Status.attacks[PT_PAWN][c] |= attacks;
            m_Status.attacks[PT_NONE][c] |= attacks;
            m_Status.nCheckers += Bitboard(attacks & BIT(theirKing)).count();
        }
        for (auto s : knights) {
            Bitboard attacks = bbs::getKnightAttacks(s);
            m_Status.attacks[PT_KNIGHT][c] |= attacks;
            m_Status.attacks[PT_NONE][c]   |= attacks;
            m_Status.nCheckers += Bitboard(attacks & BIT(theirKing)).count();
        }
        for (auto s : bishops) {
            Bitboard attacks = bbs::getBishopAttacks(s, occ);
            m_Status.attacks[PT_BISHOP][c] |= attacks;
            m_Status.attacks[PT_NONE][c]   |= attacks;
            m_Status.nCheckers += Bitboard(attacks & BIT(theirKing)).count();
        }
        for (auto s : rooks) {
            Bitboard attacks = bbs::getRookAttacks(s, occ);
            m_Status.attacks[PT_ROOK][c] |= attacks;
            m_Status.attacks[PT_NONE][c] |= attacks;
            m_Status.nCheckers += Bitboard(attacks & BIT(theirKing)).count();
        }
        for (auto s : queens) {
            Bitboard attacks = bbs::getQueenAttacks(s, occ);
            m_Status.attacks[PT_QUEEN][c] |= attacks;
            m_Status.attacks[PT_NONE][c]  |= attacks;
            m_Status.nCheckers += Bitboard(attacks & BIT(theirKing)).count();
        }
        for (auto s : kings) {
            Bitboard attacks = bbs::getKingAttacks(s);
            m_Status.attacks[PT_KING][c] |= attacks;
            m_Status.attacks[PT_NONE][c] |= attacks;
        }
    }
}

void Position::scanPins(Bitboard attackers, Square kingSquare, Color pinnedColor) {
    Bitboard occ = getCompositeBitboard();
    for (auto s : attackers) {
        Bitboard between = bbs::getSquaresBetween(s, kingSquare) & occ;

        if (between.count() != 1) {
            // More than one piece in between, none being pinned just yet.
            continue;
        }

        // A piece might be pinned
        Square pinnedSqr = *between.begin();
        Piece piece = getPieceAt(pinnedSqr);
        if (piece.getColor() == pinnedColor) {
            // Piece is being pinned
            m_Pinned.add(pinnedSqr);
            m_Pinner[pinnedSqr] = s;
        }
    }
}

void Position::updatePins() {
    m_Pinned = 0;

    for (Color c = CL_WHITE; c < CL_COUNT; ++c) {
        Color them = getOppositeColor(c);
        Square ourKing = getKingSquare(c);
        if (ourKing == SQ_INVALID) {
            // No king
            continue;
        }

        Bitboard theirBishops = getBitboard(Piece(them, PT_BISHOP));
        Bitboard theirRooks = getBitboard(Piece(them, PT_ROOK));
        Bitboard theirQueens = getBitboard(Piece(them, PT_QUEEN));

        Bitboard theirDiagonalAtks = (theirBishops | theirQueens) & bbs::getBishopAttacks(ourKing, 0);
        Bitboard theirLineAtks = (theirRooks | theirQueens) & bbs::getRookAttacks(ourKing, 0);

        scanPins(theirDiagonalAtks, ourKing, c);
        scanPins(theirLineAtks, ourKing, c);
    }
}

void Position::setColorToMove(Color c) {
    m_Status.zobrist ^= zobrist::getColorToMoveKey(m_ColorToMove);
    m_ColorToMove = c;
    m_Status.zobrist ^= zobrist::getColorToMoveKey(m_ColorToMove);
}

void Position::setCastleRights(CastlingRightsMask crm) {
    m_Status.zobrist ^= zobrist::getCastlingRightsKey(m_Status.castleRights);
    m_Status.castleRights = crm;
    m_Status.zobrist ^= zobrist::getCastlingRightsKey(m_Status.castleRights);
}

void Position::setCastleRights(Color color, Side side, bool allow) {
    // Get the bit to be set/unset
    const ui8 idx = static_cast<ui8>(color) * 2 + static_cast<ui8>(side);
    const ui8 bit = BIT(idx);

    ui8 crm = (m_Status.castleRights & (~bit)) | (allow << idx);

    setCastleRights(CastlingRightsMask(crm));
}

void Position::makeNullMove() {
    // Push current status
    m_PrevStatuses.push_back(m_Status);

    // Update ply counter
    m_PlyCount++;

    setColorToMove(getOppositeColor(m_ColorToMove));
    setEnPassantSquare(SQ_INVALID);

    updateAttacks();
    updatePins();
    refreshCastles();
}

void Position::undoNullMove() {
    m_PlyCount--;

    setColorToMove(getOppositeColor(m_ColorToMove));

    m_Status = *m_PrevStatuses.rbegin();
    m_PrevStatuses.pop_back();

    updatePins();
}

void Position::makeMove(Move move) {
    LUNA_ASSERT(isMovePseudoLegal(move),
                "Move must be pseudo legal. (tried making move " << move << " -- raw " << move.getRaw() << " -- in position " << toFen() << ")");

    // Push current status
    m_PrevStatuses.push_back(m_Status);

    // Update ply counter
    m_PlyCount++;

    // If a double push move is played, this will be overriden in the
    // handleSpecialMove method
    setEnPassantSquare(SQ_INVALID);

    m_Status.lastMove = move;

    // Update piece placement
    setPieceAt<true, false>(move.getDest(), move.getSourcePiece());
    setPieceAt<true, false>(move.getSource(), PIECE_NONE);

    // Update fifty move rule counter
    if (!move.is<MTM_CAPTURE>() && move.getSourcePiece().getType() != PT_PAWN) {
        m_Status.fiftyMoveCounter++;
    }
    else {
        m_Status.fiftyMoveCounter = 0;
    }

    Piece sourcePiece = move.getSourcePiece();
    PieceType sourcePieceType = sourcePiece.getType();
    Color sourcePieceColor = sourcePiece.getColor();

    // Check if we might have lost our castling rights on this move
    if (sourcePieceType == PT_KING) {
        // King moves always remove both castling rights
        setCastleRights(sourcePieceColor, SIDE_KING, false);
        setCastleRights(sourcePieceColor, SIDE_QUEEN, false);
    }
    else if (sourcePieceType == PT_ROOK) {
        // Rook moves remove their respective side castling rights
        if (move.getSource() == getCastleRookSrcSquare(sourcePieceColor, SIDE_QUEEN)) {
            setCastleRights(sourcePieceColor, SIDE_QUEEN, false);
        }
        else if (move.getSource() == getCastleRookSrcSquare(sourcePieceColor, SIDE_KING)) {
            setCastleRights(sourcePieceColor, SIDE_KING, false);
        }
    }

    handleSpecialMove(move);

    setColorToMove(getOppositeColor(m_ColorToMove));

    updateAttacks();
    updatePins();
    refreshCastles();
}

void Position::handleSpecialMove(Move move) {
    switch (move.getType()) {
        case MT_SIMPLE_PROMOTION:
        case MT_PROMOTION_CAPTURE: {
            // Promotion move -- replace the piece at the destination square with the desired
            // promotion piece.
            setPieceAt<true, false>(move.getDest(), Piece(getColorToMove(), move.getPromotionPiece()));
            break;
        }

        case MT_CASTLES_SHORT: {
            // Short castle
            auto king = move.getSourcePiece();

            Color color = king.getColor();

            Square prevRookSq = getCastleRookSrcSquare(color, SIDE_KING);
            Square newRookSq = getCastleRookDestSquare(king.getColor(), SIDE_KING);

            setPieceAt<true, false>(newRookSq, Piece(color, PT_ROOK));

            setPieceAt<true, false>(prevRookSq, PIECE_NONE);
            break;
        }

        case MT_CASTLES_LONG: {
            // Long castle
            auto king = move.getSourcePiece();

            Color color = king.getColor();

            Square prevRookSq = getCastleRookSrcSquare(color, SIDE_QUEEN);
            Square newRookSq = getCastleRookDestSquare(color, SIDE_QUEEN);

            setPieceAt<true, false>(newRookSq, Piece(color, PT_ROOK));

            setPieceAt<true, false>(prevRookSq, PIECE_NONE);
            break;
        }

        case MT_EN_PASSANT_CAPTURE: {
            // En passant capture
            int pushDir = getPawnStepDir(move.getSourcePiece().getColor());
            Square captureSq = move.getDest() - pushDir;
            LUNA_ASSERT(getPieceAt(captureSq).getType() == PT_PAWN,
                        "Expects the captured en passant piece to be a pawn (got "
                                << getPieceAt(captureSq).getIdentifier() << ")");
            setPieceAt<true, false>(captureSq, PIECE_NONE);
            break;
        }

        case MT_DOUBLE_PUSH: {
            // Pawn double push -- set the en passant square to the square right behind the
            // pawn that just got pushed
            int pushDir = getPawnStepDir(move.getSourcePiece().getColor());
            setEnPassantSquare(move.getDest() - pushDir);
            break;
        }

        case MT_NORMAL:
        case MT_SIMPLE_CAPTURE:
        default:
            break;
    }
}

void Position::undoMove() {
    LUNA_ASSERT(m_PrevStatuses.size() > 0, "Trying to undo move from root position node.");

    m_PlyCount--;

    Move lastMove = m_Status.lastMove;

    setPieceAt<false, false>(lastMove.getSource(), lastMove.getSourcePiece());
    setPieceAt<false, false>(lastMove.getDest(), lastMove.getDestPiece());

    m_ColorToMove = getOppositeColor(m_ColorToMove);

    handleSpecialMoveUndo();

    m_Status = *m_PrevStatuses.rbegin();
    m_PrevStatuses.pop_back();

    updatePins();
}

void Position::handleSpecialMoveUndo() {
    Move move = m_Status.lastMove;

    if (move.getType() == MT_EN_PASSANT_CAPTURE) {
        // En passant moves must replace the captured pawn back when undone
        auto epSquare = move.getDest();
        Color capturedPawnColor = getOppositeColor(move.getSourcePiece().getColor());
        Square capturedPawnSquare = epSquare + getPawnStepDir(capturedPawnColor);
        setPieceAt<false, false>(capturedPawnSquare, Piece(capturedPawnColor, PT_PAWN));
    }
    else if (move.getType() == MT_CASTLES_SHORT) {
        handleCastleUndo(SIDE_KING);
    }
    else if (move.getType() == MT_CASTLES_LONG) {
        handleCastleUndo(SIDE_QUEEN);
    }
}

void Position::handleCastleUndo(Side side) {
    Move move = m_Status.lastMove;
    Color color = move.getSourcePiece().getColor();

    Square rookSq = getCastleRookDestSquare(color, side);
    LUNA_ASSERT(getPieceAt(rookSq).getType() == PT_ROOK,
                "Expected a rook. (got " << getPieceAt(rookSq).getIdentifier() << ")");
    setPieceAt<false, false>(rookSq, PIECE_NONE);

    Square originalRookSq = getCastleRookSrcSquare(color, side);
    setPieceAt<false, false>(originalRookSq, Piece(color, PT_ROOK));
}

void Position::setEnPassantSquare(Square s) {
    if (m_Status.epSquare != SQ_INVALID) {
        m_Status.zobrist ^= zobrist::getEnPassantSquareKey(m_Status.epSquare);
    }

    m_Status.epSquare = s;

    if (m_Status.epSquare != SQ_INVALID) {
        m_Status.zobrist ^= zobrist::getEnPassantSquareKey(m_Status.epSquare);
    }
}

bool Position::isRepetitionDraw(int maxAppearances) const {
    int appearances = 1;

    for (auto it = m_PrevStatuses.crbegin(); it != m_PrevStatuses.crend(); ++it) {
        const auto& status = *it;

        if (status.lastMove.makesProgress()) {
            break;
        }
        if (status.zobrist == m_Status.zobrist) {
            appearances++;
            if (appearances == maxAppearances) {
                return true;
            }
        }
    }

    return false;
}

bool Position::colorHasSufficientMaterial(Color c) const {
    // Check for heavy pieces and pawns
    Bitboard heavyBB = getBitboard(Piece(c, PT_ROOK)) |
                       getBitboard(Piece(c, PT_QUEEN)) |
                       getBitboard(Piece(c, PT_PAWN));

    if (heavyBB.count() > 0) {
        return true;
    }

    // Check for lesser pieces
    Bitboard lightBB = getBitboard(Piece(c, PT_BISHOP)) |
                       getBitboard(Piece(c, PT_KNIGHT));
    if (lightBB.count() > 1) {
        return true;
    }

    return false;
}

bool Position::isCastlesPseudoLegal(Square kingSquare, Color c, Side castlingSide) const {
    if (!getCastleRights(c, castlingSide)) {
        // No castling rights
        return false;
    }
    constexpr Square KING_INI_SQRS[] = { SQ_E1, SQ_E8 };
    if (kingSquare != KING_INI_SQRS[c]) {
        return false;
    }

    Square rookSquare = getCastleRookSrcSquare(c, castlingSide);
    if (getPieceAt(rookSquare) != Piece(c, PT_ROOK)) {
        return false;
    }

    Bitboard theirAtks       = getAttacks(getOppositeColor(c));
    Bitboard kingCastlePath  = bbs::getKingCastlePath(c, castlingSide);
    if (theirAtks & kingCastlePath) {
        // Castling path is being attacked, don't allow.
        return false;
    }

    Bitboard occ = getCompositeBitboard();
    Bitboard innerCastlePath = bbs::getInnerCastlePath(c, castlingSide);
    if (innerCastlePath & occ) {
        // There cannot be any pieces between the king and the rook.
        return false;
    }

    return true;
}

bool Position::isMoveMovementValid(Move move) const {
    Bitboard occ   = getCompositeBitboard();
    Square src     = move.getSource();
    Square dest    = move.getDest();
    Piece srcPiece = move.getSourcePiece();
    Bitboard pMvs  = bbs::getPieceMovements(src, occ, srcPiece, getEnPassantSquare());

    return pMvs.contains(dest);
}

bool Position::isMovePseudoLegal(Move move) const {
    Square src             = move.getSource();
    Square dest            = move.getDest();
    Piece srcPiece         = move.getSourcePiece();
    Piece destPiece        = move.getDestPiece();
    Color srcPieceColor    = srcPiece.getColor();
    Color destPieceColor   = destPiece.getColor();
    PieceType srcPieceType = srcPiece.getType();

    if (src == dest) {
        // Destination can't be equal to source.
        return false;
    }

    if (srcPiece != getPieceAt(src)) {
        // Source piece must match the piece at the source square.
        return false;
    }
    if (srcPieceColor != getColorToMove()) {
        // Source piece must have the color of the current player to move.
        return false;
    }
    if (destPiece != getPieceAt(dest)) {
        return false;
    }

    // Check if move is a capture that isn't en passant.
    if (move.is<MTM_CAPTURE & ~(BIT(MT_EN_PASSANT_CAPTURE))>()) {
        if (destPiece == PIECE_NONE) {
            // Non en passant captures must have a destPiece.
            return false;
        }
        if (destPieceColor == srcPieceColor) {
            // We can only capture opposing pieces.
            return false;
        }
    }
    else {
        if (destPiece != PIECE_NONE) {
            // Non-captures (or en passants) cannot have 'dest' pieces.
            return false;
        }
    }

    // Do specific logic for special moves.
    switch (move.getType()) {
        case MT_CASTLES_LONG:
            if (srcPieceType != PT_KING) {
                // Castling can only be performed by a king.
                return false;
            }
            return isCastlesPseudoLegal(src, srcPieceColor, SIDE_QUEEN);

        case MT_CASTLES_SHORT:
            if (srcPieceType != PT_KING) {
                // Castling can only be performed by a king.
                return false;
            }
            return isCastlesPseudoLegal(src, srcPieceColor, SIDE_KING);

        case MT_PROMOTION_CAPTURE:
        case MT_SIMPLE_PROMOTION:
            if (srcPieceType != PT_PAWN) {
                // Promotions can only be performed by a pawn.
                return false;
            }
            if (getRank(dest) != getPromotionRank(srcPieceColor)) {
                // Destination rank must be the pawn's promotion rank.
                // Note that after this condition, 'isNormalMovePseudoLegal' is already going to cover
                // the requirement for 'src' to be located on the 7th rank.
                return false;
            }

            return isMoveMovementValid(move);

        case MT_EN_PASSANT_CAPTURE:
            if (srcPieceType != PT_PAWN) {
                // En passant can only be performed by a pawn.
                return false;
            }
            if (dest != getEnPassantSquare()) {
                // Destination square must be the en passant square.
                return false;
            }
            if (getPieceAt(dest - getPawnStepDir(srcPieceColor)) != Piece(getOppositeColor(srcPieceColor), PT_PAWN)) {
                // There must be an enemy pawn to be captured in en passant.
                return false;
            }
            return isMoveMovementValid(move);

        case MT_DOUBLE_PUSH:
            if (srcPieceType != PT_PAWN) {
                // Double pushes can only be performed by a pawn.
                return false;
            }
            if (destPiece != PIECE_NONE) {
                // Pawn pushes cannot capture pieces
                return false;
            }

            if (std::abs(getRank(src) - getRank(dest)) != 2) {
                // Pawn didn't move two squares.
                return false;
            }

            return isMoveMovementValid(move);

        case MT_NORMAL:
            if (srcPieceType == PT_PAWN && destPiece != PIECE_NONE) {
                return false;
            }
            return isMoveMovementValid(move);

        case MT_SIMPLE_CAPTURE:
            return isMoveMovementValid(move);

        default:
            return false;
    }
}

Square Position::getSmallestAttackerSquare(Square s, Color c) const {
    Bitboard pawns = staticanalysis::getPawnAttackers(*this, s, c);
    if (pawns != 0) {
        return *pawns.begin();
    }

    Bitboard knights = staticanalysis::getKnightAttackers(*this, s, c);
    if (knights != 0) {
        return *knights.begin();
    }

    Bitboard bishops = staticanalysis::getBishopAttackers(*this, s, c);
    if (bishops != 0) {
        return *bishops.begin();
    }

    Bitboard rooks = staticanalysis::getRookAttackers(*this, s, c);
    if (rooks != 0) {
        return *rooks.begin();
    }

    Bitboard queens = staticanalysis::getQueenAttackers(*this, s, c);
    if (queens != 0) {
        return *queens.begin();
    }

    Bitboard kings = staticanalysis::getKingAttackers(*this, s, c);
    if (kings != 0) {
        return *kings.begin();
    }

    return SQ_INVALID;
}

ChessResult Position::getResult(Color us, bool colorToMoveHasTime) const {
    Color currPlayer = getColorToMove();

    // Draw by 50 move rule
    if (is50MoveRuleDraw()) {
        return RES_DRAW_RULE50;
    }
    // Draw by repetition
    if (isRepetitionDraw()) {
        return RES_DRAW_REPETITION;
    }

    // Check for timeouts
    bool currPlayerOpponentHasMat = colorHasSufficientMaterial(getOppositeColor(currPlayer));
    if (!colorToMoveHasTime) {
        if (!currPlayerOpponentHasMat) {
            // Color to move's time has ended, but their opponent didn't have
            // enough mating material
            return RES_DRAW_TIME_NOMAT;
        }
        // Color to move's time has ended and their opponent had enough
        // mating material
        return us == currPlayer ? RES_LOSS_TIME : RES_WIN_TIME;
    }
    // Check for insufficient material draw
    bool currPlayerHasMat = colorHasSufficientMaterial(currPlayer);
    if (!currPlayerHasMat && !currPlayerOpponentHasMat) {
        return RES_DRAW_NOMAT;
    }

    // No basic draws, check for checkmates/stalemates
    MoveList legalMoves;
    movegen::generate(*this, legalMoves);
    if (legalMoves.size() > 0) {
        // Game still has legal moves and is not a draw
        return RES_UNFINISHED;
    }

    // Legal move count is zero
    if (!isCheck()) {
        // Draw by stalemate
        return RES_DRAW_STALEMATE;
    }

    // We have a checkmate on board
    return currPlayer == us ? RES_LOSS_CHECKMATE : RES_WIN_CHECKMATE;
}

Position::Position() {
    std::fill(m_Pieces.begin(), m_Pieces.end(), PIECE_NONE);
    m_PrevStatuses.reserve(64);
}

Position Position::getInitialPosition() {
    Position ret;

    ret.setPieceAt<true, false>(SQ_A1, WHITE_ROOK);
    ret.setPieceAt<true, false>(SQ_B1, WHITE_KNIGHT);
    ret.setPieceAt<true, false>(SQ_C1, WHITE_BISHOP);
    ret.setPieceAt<true, false>(SQ_D1, WHITE_QUEEN);
    ret.setPieceAt<true, false>(SQ_E1, WHITE_KING);
    ret.setPieceAt<true, false>(SQ_F1, WHITE_BISHOP);
    ret.setPieceAt<true, false>(SQ_G1, WHITE_KNIGHT);
    ret.setPieceAt<true, false>(SQ_H1, WHITE_ROOK);

    ret.setPieceAt<true, false>(SQ_A2, WHITE_PAWN);
    ret.setPieceAt<true, false>(SQ_B2, WHITE_PAWN);
    ret.setPieceAt<true, false>(SQ_C2, WHITE_PAWN);
    ret.setPieceAt<true, false>(SQ_D2, WHITE_PAWN);
    ret.setPieceAt<true, false>(SQ_E2, WHITE_PAWN);
    ret.setPieceAt<true, false>(SQ_F2, WHITE_PAWN);
    ret.setPieceAt<true, false>(SQ_G2, WHITE_PAWN);
    ret.setPieceAt<true, false>(SQ_H2, WHITE_PAWN);

    ret.setPieceAt<true, false>(SQ_A8, BLACK_ROOK);
    ret.setPieceAt<true, false>(SQ_B8, BLACK_KNIGHT);
    ret.setPieceAt<true, false>(SQ_C8, BLACK_BISHOP);
    ret.setPieceAt<true, false>(SQ_D8, BLACK_QUEEN);
    ret.setPieceAt<true, false>(SQ_E8, BLACK_KING);
    ret.setPieceAt<true, false>(SQ_F8, BLACK_BISHOP);
    ret.setPieceAt<true, false>(SQ_G8, BLACK_KNIGHT);
    ret.setPieceAt<true, false>(SQ_H8, BLACK_ROOK);

    ret.setPieceAt<true, false>(SQ_A7, BLACK_PAWN);
    ret.setPieceAt<true, false>(SQ_B7, BLACK_PAWN);
    ret.setPieceAt<true, false>(SQ_C7, BLACK_PAWN);
    ret.setPieceAt<true, false>(SQ_D7, BLACK_PAWN);
    ret.setPieceAt<true, false>(SQ_E7, BLACK_PAWN);
    ret.setPieceAt<true, false>(SQ_F7, BLACK_PAWN);
    ret.setPieceAt<true, false>(SQ_G7, BLACK_PAWN);
    ret.setPieceAt<true, false>(SQ_H7, BLACK_PAWN);

    ret.setCastleRights(CR_ALL);

    ret.updateAttacks();
    ret.updatePins();
    ret.refreshCastles();

    return ret;
}

std::string Position::toFen() const {
    std::stringstream stream;

    // Write piece placements
    for (BoardRank r = RANK_COUNT - 1; r >= RANK_1; --r) {
        int emptyCount = 0;

        for (BoardFile f = FL_A; f < FL_COUNT; ++f) {
            Square s = getSquare(f, r);
            Piece p = getPieceAt(s);

            if (p != PIECE_NONE) {
                if (emptyCount > 0) {
                    stream << emptyCount;
                    emptyCount = 0;
                }

                stream << p.getIdentifier();
            }
            else {
                emptyCount++;
            }
        }

        if (emptyCount > 0) {
            stream << emptyCount;
            emptyCount = 0;
        }

        if (r > RANK_1) {
            stream << '/';
        }
    }
    stream << ' ';

    // Write color to move
    stream << (getColorToMove() == CL_WHITE ? 'w' : 'b') << ' ';

    // Write castling rights
    if (getCastleRights() == CR_NONE) {
        stream << '-';
    }
    else {
        if (getCastleRights(CL_WHITE, SIDE_KING)) {
            stream << 'K';
        }
        if (getCastleRights(CL_WHITE, SIDE_QUEEN)) {
            stream << 'Q';
        }
        if (getCastleRights(CL_BLACK, SIDE_KING)) {
            stream << 'k';
        }
        if (getCastleRights(CL_BLACK, SIDE_QUEEN)) {
            stream << 'q';
        }
    }
    stream << ' ';

    // Write en-passant square
    Square epSquare = getEnPassantSquare();
    if (epSquare != SQ_INVALID) {
        stream << getSquareName(getEnPassantSquare());
    }
    else {
        stream << '-';
    }
    stream << ' ';

    // Write halfmove clock
    stream << m_Status.fiftyMoveCounter << ' ';

    // Write fullmove number
    stream << m_PlyCount / 2 + 1;

    return stream.str();
}

std::optional<Position> Position::fromFen(std::string_view fen) {
    try {
        char c;
        size_t i = 0;

        Position pos;

        // Read piece placements
        {
            BoardRank rank = RANK_COUNT - 1;
            BoardFile file = FL_A;

            while (true) {
                c = fen[i];

                // Interpret the read character
                if (std::isdigit(c)) {
                    // Skip some files
                    file += c - '0';

                    if (file > FL_COUNT) {
                        // File overflow, bad FEN code.
                        return std::nullopt;
                    }
                }
                else if (c == '/') {
                    // Go to next rank
                    rank--;
                    file = FL_A;
                }
                else {
                    if (file >= FL_COUNT || rank < RANK_1) {
                        // Out of bounds
                        return std::nullopt;
                    }

                    // 'c' should be a piece identifier
                    Piece p = Piece::fromIdentifier(c);

                    if (p == PIECE_NONE) {
                        // Unknown piece identifier
                        return std::nullopt;
                    }

                    pos.setPieceAt<true, false>(getSquare(file, rank), p);
                    file++;
                }

                // Check if we finished reading pieces
                if (rank <= RANK_1 && file >= FL_COUNT) {
                    break;
                }

                // We didn't finish reading the pieces, advance on the string
                i++;
                if (i >= fen.size()) {
                    // Unexpected end of string
                    return std::nullopt;
                }
            }
            i++;
        }

        pos.updateAttacks();
        pos.updatePins();
        pos.refreshCastles();

        while (std::isspace(fen[i])) i++;
        if (i >= fen.size()) {
            return pos;
        }

        // Read active color
        {
            c = fen[i];

            if (c == 'b') {
                pos.setColorToMove(CL_BLACK);
            }
            else if (c == 'w') {
                pos.setColorToMove(CL_WHITE);
            }
            else {
                // Bad active color code.
                return std::nullopt;
            }

            i++;
        }

        while (std::isspace(fen[i])) i++;
        if (i >= fen.size()) {
            return pos;
        }

        // Read castling rights
        {
            c = fen[i];
            if (c == '-') {
                // No castling rights for anyone
                pos.setCastleRights(CR_NONE);
            }
            else {
                // Some castling rights set
                while (!std::isspace(fen[i])) {
                    switch (c) {
                        case 'K':
                            pos.setCastleRights(CL_WHITE, SIDE_KING, true);
                            break;
                        case 'Q':
                            pos.setCastleRights(CL_WHITE, SIDE_QUEEN, true);
                            break;

                        case 'k':
                            pos.setCastleRights(CL_BLACK, SIDE_KING, true);
                            break;
                        case 'q':
                            pos.setCastleRights(CL_BLACK, SIDE_QUEEN, true);
                            break;

                        default:
                            return std::nullopt;
                    }

                    i++;
                    if (i >= fen.size()) {
                        // Unexpected end of string
                        return std::nullopt;
                    }
                    c = fen[i];
                }
            }

            i++;
        }

        while (std::isspace(fen[i])) i++;
        if (i >= fen.size()) {
            return pos;
        }

        // Read en-passant square
        {
            c = fen[i];

            if (c == '-') {
                pos.setEnPassantSquare(SQ_INVALID);
            }
            else {
                i++;
                if (i >= fen.size()) {
                    // Unexpected end of string
                    return std::nullopt;
                }

                char sqrStr[] = { fen[i - 1], fen[i], '\0' };
                Square s = getSquare(sqrStr);

                if (s == SQ_INVALID) {
                    // Bad en passant square
                    return std::nullopt;
                }

                pos.setEnPassantSquare(s);
            }

            i++;
        }

        while (std::isspace(fen[i])) i++;
        if (i >= fen.size()) {
            return pos;
        }

        // Read halfmove clock
        {
            if (!std::isdigit(fen[i])) {
                // Bad halfmove clock code
                return std::nullopt;
            }

            int halfmoveClock = 0;
            do {
                halfmoveClock *= 10;
                halfmoveClock += fen[i] - '0';
                i++;
            } while (std::isdigit(fen[i]));

            pos.m_Status.fiftyMoveCounter = halfmoveClock;
        }

        while (std::isspace(fen[i])) i++;
        if (i >= fen.size()) {
            return pos;
        }

        // Read fullmove number
        {
            if (!std::isdigit(fen[i])) {
                // Bad halfmove clock code
                return std::nullopt;
            }

            int fullMoveNumber = 0;
            do {
                fullMoveNumber *= 10;
                fullMoveNumber += fen[i] - '0';
                i++;
            } while (std::isdigit(fen[i]));

            pos.m_PlyCount = fullMoveNumber * 2;
            if (pos.getColorToMove() == CL_BLACK) {
                pos.m_PlyCount++;
            }
        }

        return pos;
    }
    catch (const std::exception& ex) {
        return std::nullopt;
    }
}

std::ostream& operator<<(std::ostream& stream, const Position& pos) {
    stream << "    A B C D E F G H" << std::endl;

    for (BoardRank r = RANK_8; r >= RANK_1; r--) {
        stream << static_cast<int>(r) + 1 << " [";

        for (BoardFile f = FL_A; f <= FL_H; f++) {
            Square s = getSquare(f, r);

            stream << " " << pos.getPieceAt(s).getIdentifier();
        }

        stream << " ]\n";
    }

    stream << "Side to move: " << (pos.getColorToMove() == CL_WHITE ? "White" : "Black") << std::endl;
    stream << "En passant square: " << getSquareName(pos.getEnPassantSquare()) << std::endl;
    stream << "Zobrist Key: " << pos.getZobrist() << std::endl;

    return stream;
}

}