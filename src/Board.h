#ifndef BOARD_H
#define BOARD_H

#include <cassert>
#include <cstdint>
#include <string>
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

// Inverse of pieceToChar; Piece::None for anything that isn't a piece letter
constexpr Piece charToPiece(char c) {
    for (int i = 0; i < 12; ++i) {
        if ("PNBRQKpnbrqk"[i] == c) return static_cast<Piece>(i);
    }
    return Piece::None;
}

enum class MoveFlag : uint8_t { Normal, DoublePush, EnPassant, Castle };

// Packed into 32 bits: from (6) | to (6) | promo (3) | piece (4) | captured (4) | flag (2)
class Move {
    uint32_t data = 0;

public:
    Move() = default;
    Move(int from, int to, Piece piece, Piece captured = Piece::None,
         PieceType promo = PieceType::None, MoveFlag flag = MoveFlag::Normal)
        : data(static_cast<uint32_t>(from) | static_cast<uint32_t>(to) << 6 |
               static_cast<uint32_t>(promo) << 12 | static_cast<uint32_t>(piece) << 15 |
               static_cast<uint32_t>(captured) << 19 | static_cast<uint32_t>(flag) << 23) {}

    int from() const { return data & 0x3F; }
    int to() const { return (data >> 6) & 0x3F; }
    PieceType promo() const { return static_cast<PieceType>((data >> 12) & 0x7); }
    Piece piece() const { return static_cast<Piece>((data >> 15) & 0xF); }
    Piece captured() const { return static_cast<Piece>((data >> 19) & 0xF); }
    MoveFlag flag() const { return static_cast<MoveFlag>((data >> 23) & 0x3); }
};

// Fixed-size list on the stack so move generation never allocates
class MoveList {
    Move moves[256];
    int count = 0;

public:
    void push(Move m) {
        assert(count < 256);
        moves[count++] = m;
    }
    int size() const { return count; }
    bool empty() const { return count == 0; }
    const Move &operator[](int i) const { return moves[i]; }
    const Move *begin() const { return moves; }
    const Move *end() const { return moves + count; }
};

// What makeMove can't recover from the move itself
struct Undo {
    uint8_t castling;
    int8_t epSq;
    uint16_t halfmoveClock;
};

class Board {
    Bitboard pieceBB[12] = {};

    // Kept in sync with the piece bitboards so lookups are a single read
    Bitboard whiteOcc = 0;
    Bitboard blackOcc = 0;
    Bitboard allOcc = 0;
    Piece mailbox[64];

    // Bits: 1 white king side, 2 white queen side, 4 black king side, 8 black queen side
    uint8_t castling = 0;
    int epSq = -1;
    bool whiteTurn = true;
    // Plies since the last capture or pawn move, for the fifty-move rule
    int halfmoveClock = 0;
    uint64_t zobrist = 0;
    // Hash of every earlier position in the game, oldest first, for repetition detection
    std::vector<uint64_t> history;

public:
    Board();

    static bool getBit(Bitboard bb, int square);
    static void setBit(Bitboard &bb, int square);
    static void clearBit(Bitboard &bb, int square);

    void printBoard() const;
    void initPieces();
    // Returns false and leaves the board unchanged if the FEN is malformed
    bool loadFen(const std::string &fen);
    Piece pieceAt(int square) const;
    bool isWhiteTurn() const;
    uint64_t hash() const;
    // Recomputes the hash from scratch, to check the incremental one
    uint64_t computeHash() const;

    bool isSquareAttacked(int sq, bool byWhite) const;
    bool inCheck(bool white) const;
    // The rest of these are for the side to move
    bool inCheck() const;
    MoveList pseudoLegalMoves() const;
    MoveList legalMoves() const;
    bool hasLegalMove() const;
    bool isCheckmate() const;
    bool isStalemate() const;
    bool insufficientMaterial() const;
    bool isFiftyMoveDraw() const;
    // How many times the current position occurred before; 2 or more is threefold repetition
    int repetitions() const;
    Undo makeMove(Move m);
    void unmakeMove(Move m, const Undo &undo);

private:
    void collectLegalMoves(MoveList &legal, bool stopAtFirst) const;
    bool leavesKingAttacked(Move m, int kingSq) const;
    bool isSquareAttacked(int sq, bool byWhite, Bitboard occ, Bitboard ignore) const;
    void addMoves(MoveList &moves, int from, Piece piece, Bitboard targets) const;
    int kingSquare(bool white) const;
    Bitboard pinnedPieces(bool white, int kingSq) const;
    void movePiece(int from, int to, Piece piece);
    void removePiece(int square, Piece piece);
    void putPiece(int square, Piece piece);
    void syncFromBitboards();
    Bitboard &bitboardFor(Piece piece);
    Bitboard bitboardFor(Piece piece) const;
};

#endif
