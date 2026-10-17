#ifndef BOARD_H
#define BOARD_H

#include <cstdint>
#include <vector>

using Bitboard = uint64_t;

struct Move {
    int from;
    int to;
    char promo;
    Move(int f = 0, int t = 0, char p = 0) : from(f), to(t), promo(p) {}
};

class Board {
    Bitboard whitePawns = 0;
    Bitboard whiteKnights = 0;
    Bitboard whiteRooks = 0;
    Bitboard whiteBishops = 0;
    Bitboard whiteKing = 0;
    Bitboard whiteQueens = 0;
    Bitboard blackPawns = 0;
    Bitboard blackKnights = 0;
    Bitboard blackRooks = 0;
    Bitboard blackBishops = 0;
    Bitboard blackKing = 0;
    Bitboard blackQueens = 0;

    bool whiteCastleK = false;
    bool whiteCastleQ = false;
    bool blackCastleK = false;
    bool blackCastleQ = false;

    int epSq = -1;

public:
    static bool getBit(Bitboard bb, int square);
    static void setBit(Bitboard &bb, int square);
    static void clearBit(Bitboard &bb, int square);
    static bool isWhitePiece(char c);

    void printBoard() const;
    void initPieces();
    void movePiece(int frSq, int toSq);
    char pieceCharAt(int square) const;

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
    static void moveOnBitBoard(Bitboard &bb, int frSq, int toSq);
    void addStep(std::vector<Move> &moves, int sq, int nf, int nr, bool white) const;
    void clearSquare(int square);
    Bitboard &bitboardFor(char piece);
};

#endif
