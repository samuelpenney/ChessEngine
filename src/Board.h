#ifndef BOARD_H
#define BOARD_H

#include <cstdint>
#include <vector>

using Bitboard = uint64_t;

enum class PieceType : uint8_t { Pawn, Knight, Bishop, Rook, Queen, King, None };

// White pieces come first, in the same order as PieceType
enum class Piece : uint8_t {
    WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing,
    BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing,
    None
};

constexpr Piece makePiece(PieceType type, bool white) {
    return static_cast<Piece>(static_cast<int>(type) + (white ? 0 : 6));
}

// Only meaningful for real pieces, not Piece::None
constexpr PieceType pieceType(Piece p) {
    return static_cast<PieceType>(static_cast<int>(p) % 6);
}

constexpr bool isWhite(Piece p) {
    return p < Piece::BlackPawn;
}

// For printing: 'P', 'n', ... and '.' for an empty square
constexpr char pieceToChar(Piece p) {
    return "PNBRQKpnbrqk."[static_cast<int>(p)];
}

struct Move {
    int from;
    int to;
    PieceType promo;
    Move(int f = 0, int t = 0, PieceType p = PieceType::None) : from(f), to(t), promo(p) {}
};

class Board {
    Bitboard pieceBB[12] = {};

    // Kept in sync with the piece bitboards so lookups are a single read
    Bitboard whiteOcc = 0;
    Bitboard blackOcc = 0;
    Bitboard allOcc = 0;
    Piece mailbox[64];

    bool whiteCastleK = false;
    bool whiteCastleQ = false;
    bool blackCastleK = false;
    bool blackCastleQ = false;

    int epSq = -1;

public:
    Board();

    static bool getBit(Bitboard bb, int square);
    static void setBit(Bitboard &bb, int square);
    static void clearBit(Bitboard &bb, int square);

    void printBoard() const;
    void initPieces();
    void movePiece(int frSq, int toSq);
    Piece pieceAt(int square) const;

    bool isSquareAttacked(int sq, bool byWhite) const;
    bool inCheck(bool white) const;
    std::vector<Move> pseudoLegalMoves(bool white) const;
    std::vector<Move> legalMoves(bool white) const;
    bool isCheckmate(bool white) const;
    bool isStalemate(bool white) const;
    bool insufficientMaterial() const;
    void makeMove(const Move &m);

private:
    void slide(std::vector<Move> &moves, int sq, bool white, bool diag, bool straight) const;
    static bool inBounds(int f, int r);
    static int popCount(Bitboard bb);
    int kingSquare(bool white) const;
    void addStep(std::vector<Move> &moves, int sq, int nf, int nr, bool white) const;
    void clearSquare(int square);
    void putPiece(int square, Piece piece);
    void syncFromBitboards();
    Bitboard &bitboardFor(Piece piece);
    Bitboard bitboardFor(Piece piece) const;
};

#endif
