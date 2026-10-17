#include "Board.h"
#include <iostream>
#include <sstream>

namespace {

// Ray directions. The first four increase the square index, the last four decrease it.
enum Dir { North, East, NorthEast, NorthWest, South, West, SouthEast, SouthWest };

struct AttackTables {
    Bitboard knight[64];
    Bitboard king[64];
    Bitboard pawn[2][64];  // [0] white, [1] black: squares a pawn on sq attacks
    Bitboard ray[8][64];   // every square from sq to the edge in a direction, sq excluded
};

constexpr AttackTables buildAttackTables() {
    AttackTables t{};
    const int kn[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    const int dirs[8][2] = {{0,1},{1,0},{1,1},{-1,1},{0,-1},{-1,0},{1,-1},{-1,-1}};
    auto inBounds = [](int f, int r) { return f >= 0 && f < 8 && r >= 0 && r < 8; };
    auto bit = [](int f, int r) { return 1ULL << (r * 8 + f); };

    for (int sq = 0; sq < 64; ++sq) {
        const int f = sq & 7, r = sq >> 3;
        for (const auto &d : kn) {
            if (inBounds(f + d[0], r + d[1])) t.knight[sq] |= bit(f + d[0], r + d[1]);
        }
        for (int i = 0; i < 8; ++i) {
            int nf = f + dirs[i][0], nr = r + dirs[i][1];
            if (inBounds(nf, nr)) t.king[sq] |= bit(nf, nr);
            while (inBounds(nf, nr)) {
                t.ray[i][sq] |= bit(nf, nr);
                nf += dirs[i][0];
                nr += dirs[i][1];
            }
        }
        for (int df = -1; df <= 1; df += 2) {
            if (inBounds(f + df, r + 1)) t.pawn[0][sq] |= bit(f + df, r + 1);
            if (inBounds(f + df, r - 1)) t.pawn[1][sq] |= bit(f + df, r - 1);
        }
    }
    return t;
}

constexpr AttackTables tables = buildAttackTables();

constexpr uint8_t WhiteKingSide = 1, WhiteQueenSide = 2, BlackKingSide = 4, BlackQueenSide = 8;

// Castling rights that survive a move touching each square (king or rook moving, or a rook being taken)
struct CastlingMasks {
    uint8_t keep[64];
};

constexpr CastlingMasks buildCastlingMasks() {
    CastlingMasks m{};
    for (int sq = 0; sq < 64; ++sq) m.keep[sq] = 0xF;
    m.keep[0] = static_cast<uint8_t>(~WhiteQueenSide & 0xF);
    m.keep[4] = static_cast<uint8_t>(~(WhiteKingSide | WhiteQueenSide) & 0xF);
    m.keep[7] = static_cast<uint8_t>(~WhiteKingSide & 0xF);
    m.keep[56] = static_cast<uint8_t>(~BlackQueenSide & 0xF);
    m.keep[60] = static_cast<uint8_t>(~(BlackKingSide | BlackQueenSide) & 0xF);
    m.keep[63] = static_cast<uint8_t>(~BlackKingSide & 0xF);
    return m;
}

constexpr CastlingMasks castlingMasks = buildCastlingMasks();

// Random numbers XORed together to hash a position: one per piece on each square,
// one per set of castling rights, one per en passant file, and one for black to move
struct ZobristKeys {
    uint64_t piece[12][64];
    uint64_t castling[16];
    uint64_t epFile[8];
    uint64_t blackToMove;
};

constexpr ZobristKeys buildZobristKeys() {
    ZobristKeys z{};
    // splitmix64, so the keys are the same on every run
    uint64_t state = 0;
    auto next = [&state]() {
        uint64_t x = (state += 0x9E3779B97F4A7C15ULL);
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
        return x ^ (x >> 31);
    };
    for (auto &squares : z.piece) {
        for (uint64_t &key : squares) key = next();
    }
    for (uint64_t &key : z.castling) key = next();
    for (uint64_t &key : z.epFile) key = next();
    z.blackToMove = next();
    return z;
}

constexpr ZobristKeys zobristKeys = buildZobristKeys();

// Ray up to and including the first blocker
Bitboard rayAttacks(int sq, int dir, Bitboard occ) {
    Bitboard attacks = tables.ray[dir][sq];
    Bitboard blockers = attacks & occ;
    if (blockers) {
        int first = dir < South ? __builtin_ctzll(blockers) : 63 - __builtin_clzll(blockers);
        attacks ^= tables.ray[dir][first];
    }
    return attacks;
}

Bitboard rookAttacks(int sq, Bitboard occ) {
    return rayAttacks(sq, North, occ) | rayAttacks(sq, East, occ) |
           rayAttacks(sq, South, occ) | rayAttacks(sq, West, occ);
}

Bitboard bishopAttacks(int sq, Bitboard occ) {
    return rayAttacks(sq, NorthEast, occ) | rayAttacks(sq, NorthWest, occ) |
           rayAttacks(sq, SouthEast, occ) | rayAttacks(sq, SouthWest, occ);
}

}

Board::Board() {
    syncFromBitboards();
}

bool Board::getBit(Bitboard bb, int square) {
    return (bb >> square) & 1ULL;
}

void Board::setBit(Bitboard &bb, int square) {
    bb |= 1ULL << square;
}

void Board::clearBit(Bitboard &bb, int square) {
    bb &= ~(1ULL << square);
}

void Board::printBoard() const {
    std::cout << "\n    a b c d e f g h\n";
    std::cout << "    ---------------\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << (rank + 1) << " |" << " ";
        for (int file = 0; file < 8; ++file) {
            int square = rank * 8 + file;
            std::cout << pieceToChar(pieceAt(square)) << ' ';
        }
        std::cout << '\n';
    }
}

void Board::initPieces() {
    bitboardFor(Piece::WhitePawn) = 0x000000000000FF00ULL;
    bitboardFor(Piece::WhiteKnight) = 0x0000000000000042ULL;
    bitboardFor(Piece::WhiteRook) = 0x0000000000000081ULL;
    bitboardFor(Piece::WhiteBishop) = 0x0000000000000024ULL;
    bitboardFor(Piece::WhiteQueen) = 0x0000000000000008ULL;
    bitboardFor(Piece::WhiteKing) = 0x0000000000000010ULL;

    bitboardFor(Piece::BlackPawn) = 0x00FF000000000000ULL;
    bitboardFor(Piece::BlackKnight) = 0x4200000000000000ULL;
    bitboardFor(Piece::BlackRook) = 0x8100000000000000ULL;
    bitboardFor(Piece::BlackBishop) = 0x2400000000000000ULL;
    bitboardFor(Piece::BlackQueen) = 0x0800000000000000ULL;
    bitboardFor(Piece::BlackKing) = 0x1000000000000000ULL;

    castling = WhiteKingSide | WhiteQueenSide | BlackKingSide | BlackQueenSide;
    epSq = -1;
    whiteTurn = true;
    halfmoveClock = 0;
    history.clear();
    syncFromBitboards();
    zobrist = computeHash();
}

bool Board::loadFen(const std::string &fen) {
    std::istringstream in(fen);
    std::string placement, side, rights = "-", ep = "-";
    int halfmove = 0;
    if (!(in >> placement >> side)) return false;
    in >> rights >> ep >> halfmove;  // optional; the fullmove number isn't tracked
    if (halfmove < 0) return false;

    Board b;
    int rank = 7, file = 0;
    for (char c : placement) {
        if (c == '/') {
            if (file != 8 || rank == 0) return false;
            --rank;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8) return false;
        } else {
            Piece p = charToPiece(c);
            if (p == Piece::None || file > 7) return false;
            setBit(b.bitboardFor(p), rank * 8 + file);
            ++file;
        }
    }
    if (rank != 0 || file != 8) return false;
    if (__builtin_popcountll(b.bitboardFor(Piece::WhiteKing)) != 1 ||
        __builtin_popcountll(b.bitboardFor(Piece::BlackKing)) != 1) return false;

    if (side != "w" && side != "b") return false;
    const bool white = side == "w";

    if (rights != "-") {
        for (char c : rights) {
            switch (c) {
                case 'K': b.castling |= WhiteKingSide; break;
                case 'Q': b.castling |= WhiteQueenSide; break;
                case 'k': b.castling |= BlackKingSide; break;
                case 'q': b.castling |= BlackQueenSide; break;
                default: return false;
            }
        }
    }

    b.syncFromBitboards();

    if (ep != "-") {
        if (ep.size() != 2 || ep[0] < 'a' || ep[0] > 'h' || ep[1] != (white ? '6' : '3')) return false;
        const int sq = (ep[1] - '1') * 8 + (ep[0] - 'a');
        // Some FENs list the square after every double step; only keep it if the pushed pawn is
        // really there and a pawn can take it, the same as makeMove does
        const int pawnSq = white ? sq - 8 : sq + 8;
        if (b.mailbox[sq] == Piece::None && b.mailbox[pawnSq] == makePiece(PieceType::Pawn, !white) &&
            (tables.pawn[white ? 1 : 0][sq] & b.bitboardFor(makePiece(PieceType::Pawn, white)))) {
            b.epSq = sq;
        }
    }

    b.whiteTurn = white;
    b.halfmoveClock = halfmove;
    b.zobrist = b.computeHash();
    *this = b;
    return true;
}

Piece Board::pieceAt(int square) const {
    return mailbox[square];
}

bool Board::isWhiteTurn() const {
    return whiteTurn;
}

uint64_t Board::hash() const {
    return zobrist;
}

uint64_t Board::computeHash() const {
    uint64_t h = 0;
    for (int i = 0; i < 12; ++i) {
        for (Bitboard b = pieceBB[i]; b; b &= b - 1) h ^= zobristKeys.piece[i][__builtin_ctzll(b)];
    }
    h ^= zobristKeys.castling[castling];
    if (epSq >= 0) h ^= zobristKeys.epFile[epSq & 7];
    if (!whiteTurn) h ^= zobristKeys.blackToMove;
    return h;
}

bool Board::isSquareAttacked(int sq, bool byWhite) const {
    return isSquareAttacked(sq, byWhite, allOcc, 0);
}

bool Board::inCheck(bool white) const {
    int k = kingSquare(white);
    if (k < 0) return false;
    return isSquareAttacked(k, !white);
}

bool Board::inCheck() const {
    return inCheck(whiteTurn);
}

MoveList Board::pseudoLegalMoves() const {
    const bool white = whiteTurn;
    MoveList moves;
    const Bitboard own = white ? whiteOcc : blackOcc;
    const Bitboard enemy = white ? blackOcc : whiteOcc;

    for (Bitboard pieces = own; pieces; pieces &= pieces - 1) {
        const int sq = __builtin_ctzll(pieces);
        const Piece piece = mailbox[sq];
        Bitboard targets = 0;

        switch (pieceType(piece)) {
            case PieceType::Pawn: {
                const int dir = white ? 8 : -8;
                const Bitboard startRank = white ? 0x000000000000FF00ULL : 0x00FF000000000000ULL;
                const Bitboard lastRank = white ? 0xFF00000000000000ULL : 0x00000000000000FFULL;
                const Bitboard attacks = tables.pawn[white ? 0 : 1][sq];

                Bitboard pawnTargets = attacks & enemy;
                int one = sq + dir;
                if (!getBit(allOcc, one)) {
                    setBit(pawnTargets, one);
                    int two = one + dir;
                    if (getBit(startRank, sq) && !getBit(allOcc, two)) {
                        moves.push(Move(sq, two, piece, Piece::None, PieceType::None, MoveFlag::DoublePush));
                    }
                }
                if (epSq >= 0 && getBit(attacks, epSq)) {
                    moves.push(Move(sq, epSq, piece, makePiece(PieceType::Pawn, !white),
                                    PieceType::None, MoveFlag::EnPassant));
                }
                for (; pawnTargets; pawnTargets &= pawnTargets - 1) {
                    int to = __builtin_ctzll(pawnTargets);
                    if (getBit(lastRank, to)) {
                        for (PieceType p : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
                            moves.push(Move(sq, to, piece, mailbox[to], p));
                        }
                    } else {
                        moves.push(Move(sq, to, piece, mailbox[to]));
                    }
                }
                continue;
            }
            case PieceType::Knight: targets = tables.knight[sq]; break;
            case PieceType::Bishop: targets = bishopAttacks(sq, allOcc); break;
            case PieceType::Rook: targets = rookAttacks(sq, allOcc); break;
            case PieceType::Queen: targets = rookAttacks(sq, allOcc) | bishopAttacks(sq, allOcc); break;
            case PieceType::King: {
                targets = tables.king[sq];
                int base = white ? 0 : 56;
                Bitboard rooks = bitboardFor(makePiece(PieceType::Rook, white));
                bool canK = castling & (white ? WhiteKingSide : BlackKingSide);
                bool canQ = castling & (white ? WhiteQueenSide : BlackQueenSide);
                if (sq == base + 4 && !isSquareAttacked(sq, !white)) {
                    const Bitboard kBetween = 0x60ULL << base;  // f and g files
                    const Bitboard qBetween = 0x0EULL << base;  // b, c and d files
                    if (canK && getBit(rooks, base + 7) && !(allOcc & kBetween) &&
                        !isSquareAttacked(base + 5, !white) &&
                        !isSquareAttacked(base + 6, !white)) {

                        moves.push(Move(sq, base + 6, piece, Piece::None, PieceType::None, MoveFlag::Castle));
                    }
                    if (canQ && getBit(rooks, base + 0) && !(allOcc & qBetween) &&
                        !isSquareAttacked(base + 3, !white) &&
                        !isSquareAttacked(base + 2, !white)) {

                        moves.push(Move(sq, base + 2, piece, Piece::None, PieceType::None, MoveFlag::Castle));
                    }
                }
                break;
            }
            case PieceType::None: break;
        }
        addMoves(moves, sq, piece, targets & ~own);
    }
    return moves;
}

MoveList Board::legalMoves() const {
    MoveList legal;
    collectLegalMoves(legal, false);
    return legal;
}

bool Board::hasLegalMove() const {
    MoveList legal;
    collectLegalMoves(legal, true);
    return !legal.empty();
}

bool Board::isCheckmate() const {
    return inCheck() && !hasLegalMove();
}

bool Board::isStalemate() const {
    return !inCheck() && !hasLegalMove();
}

bool Board::insufficientMaterial() const {
    for (PieceType t : {PieceType::Pawn, PieceType::Rook, PieceType::Queen}) {
        if (bitboardFor(makePiece(t, true)) || bitboardFor(makePiece(t, false))) return false;
    }
    return __builtin_popcountll(bitboardFor(Piece::WhiteKnight) | bitboardFor(Piece::WhiteBishop) |
                    bitboardFor(Piece::BlackKnight) | bitboardFor(Piece::BlackBishop)) <= 1;
}

bool Board::isFiftyMoveDraw() const {
    return halfmoveClock >= 100;
}

int Board::repetitions() const {
    // Only positions since the last capture or pawn move can come back, and only every other ply
    // has the same side to move
    const int n = static_cast<int>(history.size());
    const int oldest = n > halfmoveClock ? n - halfmoveClock : 0;
    int count = 0;
    for (int i = n - 2; i >= oldest; i -= 2) {
        if (history[i] == zobrist) ++count;
    }
    return count;
}

Undo Board::makeMove(Move m) {
    const Undo undo{castling, static_cast<int8_t>(epSq), static_cast<uint16_t>(halfmoveClock)};
    history.push_back(zobrist);
    const int from = m.from(), to = m.to();
    const Piece piece = m.piece();
    const bool white = whiteTurn;

    if (m.captured() != Piece::None) {
        // en passant takes the pawn beside the destination
        int capturedSq = m.flag() == MoveFlag::EnPassant ? (white ? to - 8 : to + 8) : to;
        removePiece(capturedSq, m.captured());
    }
    movePiece(from, to, piece);

    if (m.promo() != PieceType::None) {
        removePiece(to, piece);
        putPiece(to, makePiece(m.promo(), white));
    }

    // King moves two files, bringing the rook across
    if (m.flag() == MoveFlag::Castle) {
        Piece rook = makePiece(PieceType::Rook, white);
        if (to > from) {
            movePiece(from + 3, from + 1, rook);
        } else {
            movePiece(from - 4, from - 1, rook);
        }
    }

    zobrist ^= zobristKeys.castling[castling];
    castling &= castlingMasks.keep[from] & castlingMasks.keep[to];
    zobrist ^= zobristKeys.castling[castling];

    //en passant target is only available for one move after the double step btw
    if (epSq >= 0) zobrist ^= zobristKeys.epFile[epSq & 7];
    epSq = -1;
    if (m.flag() == MoveFlag::DoublePush) {
        // Only set when an enemy pawn could take, so otherwise identical positions hash the same
        const int ep = (from + to) / 2;
        if (tables.pawn[white ? 0 : 1][ep] & bitboardFor(makePiece(PieceType::Pawn, !white))) {
            epSq = ep;
            zobrist ^= zobristKeys.epFile[ep & 7];
        }
    }

    halfmoveClock = pieceType(piece) == PieceType::Pawn || m.captured() != Piece::None ? 0 : halfmoveClock + 1;
    whiteTurn = !whiteTurn;
    zobrist ^= zobristKeys.blackToMove;
    return undo;
}

void Board::unmakeMove(Move m, const Undo &undo) {
    const int from = m.from(), to = m.to();
    const Piece piece = m.piece();
    whiteTurn = !whiteTurn;
    const bool white = whiteTurn;

    if (m.flag() == MoveFlag::Castle) {
        Piece rook = makePiece(PieceType::Rook, white);
        if (to > from) {
            movePiece(from + 1, from + 3, rook);
        } else {
            movePiece(from - 1, from - 4, rook);
        }
    }

    if (m.promo() != PieceType::None) {
        removePiece(to, makePiece(m.promo(), white));
        putPiece(to, piece);
    }
    movePiece(to, from, piece);

    if (m.captured() != Piece::None) {
        int capturedSq = m.flag() == MoveFlag::EnPassant ? (white ? to - 8 : to + 8) : to;
        putPiece(capturedSq, m.captured());
    }

    castling = undo.castling;
    epSq = undo.epSq;
    halfmoveClock = undo.halfmoveClock;
    // The piece helpers changed the hash as they went, but the saved one is exact
    zobrist = history.back();
    history.pop_back();
}

// Private methods

void Board::collectLegalMoves(MoveList &legal, bool stopAtFirst) const {
    const bool white = whiteTurn;
    const MoveList pseudo = pseudoLegalMoves();
    const int k = kingSquare(white);
    const bool check = k >= 0 && isSquareAttacked(k, !white);
    const Bitboard pinned = k >= 0 ? pinnedPieces(white, k) : 0;
    for (Move m : pseudo) {
        // Out of check, only king moves, en passant and pinned pieces can expose the king
        if (k >= 0 && (check || m.from() == k || m.flag() == MoveFlag::EnPassant || getBit(pinned, m.from())) &&
            leavesKingAttacked(m, k)) {
            continue;
        }
        legal.push(m);
        if (stopAtFirst) return;
    }
}

// Works out from the occupancy alone whether m leaves the mover's king attacked, so the board
// doesn't have to be copied to try the move. The castling rook isn't moved, but it can't
// change whether the king's new square is attacked.
bool Board::leavesKingAttacked(Move m, int kingSq) const {
    const int from = m.from(), to = m.to();
    Bitboard captured = 0;
    if (m.captured() != Piece::None) {
        captured = 1ULL << (m.flag() == MoveFlag::EnPassant ? (whiteTurn ? to - 8 : to + 8) : to);
    }
    const Bitboard occ = (allOcc & ~(1ULL << from) & ~captured) | (1ULL << to);
    return isSquareAttacked(from == kingSq ? to : kingSq, !whiteTurn, occ, captured);
}

// Attack test against a hypothetical occupancy, with the pieces on ignore treated as gone
bool Board::isSquareAttacked(int sq, bool byWhite, Bitboard occ, Bitboard ignore) const {
    const Bitboard keep = ~ignore;
    const Bitboard queens = bitboardFor(makePiece(PieceType::Queen, byWhite));
    const Bitboard rooks = (bitboardFor(makePiece(PieceType::Rook, byWhite)) | queens) & keep;
    const Bitboard bishops = (bitboardFor(makePiece(PieceType::Bishop, byWhite)) | queens) & keep;

    // A pawn of the other colour on sq attacks exactly the squares an attacking pawn could stand on
    return (tables.pawn[byWhite ? 1 : 0][sq] & bitboardFor(makePiece(PieceType::Pawn, byWhite)) & keep) ||
           (tables.knight[sq] & bitboardFor(makePiece(PieceType::Knight, byWhite)) & keep) ||
           (tables.king[sq] & bitboardFor(makePiece(PieceType::King, byWhite))) ||
           (rookAttacks(sq, occ) & rooks) ||
           (bishopAttacks(sq, occ) & bishops);
}

void Board::addMoves(MoveList &moves, int from, Piece piece, Bitboard targets) const {
    for (; targets; targets &= targets - 1) {
        int to = __builtin_ctzll(targets);
        moves.push(Move(from, to, piece, mailbox[to]));
    }
}

int Board::kingSquare(bool white) const {
    Bitboard k = bitboardFor(makePiece(PieceType::King, white));
    return k ? __builtin_ctzll(k) : -1;
}

// Own pieces that are the only thing between the king and an enemy slider
Bitboard Board::pinnedPieces(bool white, int kingSq) const {
    const Bitboard own = white ? whiteOcc : blackOcc;
    const Bitboard queens = bitboardFor(makePiece(PieceType::Queen, !white));
    const Bitboard rooks = bitboardFor(makePiece(PieceType::Rook, !white)) | queens;
    const Bitboard bishops = bitboardFor(makePiece(PieceType::Bishop, !white)) | queens;

    Bitboard pinned = 0;
    for (int dir = 0; dir < 8; ++dir) {
        const bool straight = dir == North || dir == East || dir == South || dir == West;
        const Bitboard ray = tables.ray[dir][kingSq];
        const Bitboard snipers = ray & (straight ? rooks : bishops);
        if (!snipers) continue;

        int nearest = dir < South ? __builtin_ctzll(snipers) : 63 - __builtin_clzll(snipers);
        Bitboard blockers = ray & ~tables.ray[dir][nearest] & ~(1ULL << nearest) & allOcc;
        bool single = blockers && !(blockers & (blockers - 1));
        if (single && (blockers & own)) pinned |= blockers;
    }
    return pinned;
}

// Destination must be empty
void Board::movePiece(int from, int to, Piece piece) {
    const Bitboard fromTo = (1ULL << from) | (1ULL << to);
    bitboardFor(piece) ^= fromTo;
    (isWhite(piece) ? whiteOcc : blackOcc) ^= fromTo;
    allOcc ^= fromTo;
    mailbox[from] = Piece::None;
    mailbox[to] = piece;
    zobrist ^= zobristKeys.piece[static_cast<int>(piece)][from] ^ zobristKeys.piece[static_cast<int>(piece)][to];
}

void Board::removePiece(int square, Piece piece) {
    clearBit(bitboardFor(piece), square);
    clearBit(isWhite(piece) ? whiteOcc : blackOcc, square);
    clearBit(allOcc, square);
    mailbox[square] = Piece::None;
    zobrist ^= zobristKeys.piece[static_cast<int>(piece)][square];
}

// Square must be empty
void Board::putPiece(int square, Piece piece) {
    setBit(bitboardFor(piece), square);
    setBit(isWhite(piece) ? whiteOcc : blackOcc, square);
    setBit(allOcc, square);
    mailbox[square] = piece;
    zobrist ^= zobristKeys.piece[static_cast<int>(piece)][square];
}

// Rebuild mailbox and occupancy after the piece bitboards are set directly
void Board::syncFromBitboards() {
    whiteOcc = blackOcc = 0;
    for (int sq = 0; sq < 64; ++sq) mailbox[sq] = Piece::None;
    for (int i = 0; i < 12; ++i) {
        Piece piece = static_cast<Piece>(i);
        (isWhite(piece) ? whiteOcc : blackOcc) |= pieceBB[i];
        for (Bitboard b = pieceBB[i]; b; b &= b - 1) mailbox[__builtin_ctzll(b)] = piece;
    }
    allOcc = whiteOcc | blackOcc;
}

Bitboard &Board::bitboardFor(Piece piece) {
    return pieceBB[static_cast<int>(piece)];
}

Bitboard Board::bitboardFor(Piece piece) const {
    return pieceBB[static_cast<int>(piece)];
}
