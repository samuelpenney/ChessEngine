#include "Board.h"
#include <iostream>
#include <cmath>

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

    whiteCastleK = whiteCastleQ = blackCastleK = blackCastleQ = true;
    epSq = -1;
    syncFromBitboards();
}

void Board::movePiece(int frSq, int toSq) {
    Piece piece = mailbox[frSq];
    if (piece == Piece::None) {
        std::cerr << "No piece on from square " << frSq << std::endl;
        return;
    }

    clearSquare(toSq);
    clearSquare(frSq);
    putPiece(toSq, piece);
}

Piece Board::pieceAt(int square) const {
    return mailbox[square];
}

bool Board::isSquareAttacked(int sq, bool byWhite) const {
    const int f = sq & 7, r = sq >> 3;
    const Piece P = makePiece(PieceType::Pawn, byWhite);
    const Piece N = makePiece(PieceType::Knight, byWhite);
    const Piece B = makePiece(PieceType::Bishop, byWhite);
    const Piece R = makePiece(PieceType::Rook, byWhite);
    const Piece Q = makePiece(PieceType::Queen, byWhite);
    const Piece K = makePiece(PieceType::King, byWhite);

    //Pawns
    int pr = byWhite ? r - 1 : r + 1;
    for (int df = -1; df <= 1; df += 2) {
        int pf = f + df;
        if (inBounds(pf, pr) && pieceAt(pr * 8 + pf) == P) return true;
    }

    // Knights
    static const int kn[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
    for (auto &d : kn) {
        int nf = f + d[0], nr = r + d[1];
        if (inBounds(nf, nr) && pieceAt(nr * 8 + nf) == N) return true;
    }

    //King
    for (int df = -1; df <= 1; ++df) {
        for (int dr = -1; dr <= 1; ++dr) {
            if (df == 0 && dr == 0) continue;
            int nf = f + df, nr = r + dr;
            if (inBounds(nf, nr) && pieceAt(nr * 8 + nf) == K) return true;
        }
    }

    // rook/queen straights & bishop/queen diagonals
    static const int straight[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    static const int diagonal[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto &d : straight) {
        int nf = f + d[0], nr = r + d[1];
        while (inBounds(nf, nr)) {
            Piece c = pieceAt(nr * 8 + nf);
            if (c != Piece::None) {
                if (c == R || c == Q) return true; break;
            }
            nf += d[0];
            nr += d[1];
        }
    }
    for (auto &d : diagonal) {
        int nf = f + d[0], nr = r + d[1];
        while (inBounds(nf, nr)) {
            Piece c = pieceAt(nr * 8 + nf);
            if (c != Piece::None) {
                if (c == B || c == Q) return true; break;
            }
            nf += d[0];
            nr += d[1];
        }
    }
    return false;
}

bool Board::inCheck(bool white) const {
    int k = kingSquare(white);
    if (k < 0) return false;
    return isSquareAttacked(k, !white);
}

std::vector<Move> Board::pseudoLegalMoves(bool white) const {
    std::vector<Move> moves;
    for (Bitboard own = white ? whiteOcc : blackOcc; own; own &= own - 1) {
        const int sq = __builtin_ctzll(own);
        const Piece c = mailbox[sq];
        const int f = sq & 7, r = sq >> 3;

        switch (pieceType(c)) {
            case PieceType::Pawn: {
                int dir = white ? 1 : -1;
                int startRank = white ? 1 : 6;
                int lastRank = white ? 7 : 0;
                auto add = [&](int to) {
                    if ((to >> 3) == lastRank) {
                        for (PieceType p : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
                            moves.emplace_back(sq, to, p);
                        }
                    } else {
                        moves.emplace_back(sq, to);
                    }
                };
                int nr = r + dir;
                if (nr < 0 || nr > 7) break;
                int one = nr * 8 + f;
                if (pieceAt(one) == Piece::None) {
                    add(one);
                    int two = one + dir * 8;
                    if (r == startRank && pieceAt(two) == Piece::None) {
                        moves.emplace_back(sq, two);
                    }
                }
                for (int df = -1; df <= 1; df += 2) {
                    int nf = f + df;
                    if (nf < 0 || nf > 7) continue;
                    int to = nr * 8 + nf;
                    Piece tc = pieceAt(to);
                    if (tc != Piece::None && isWhite(tc) != white) {
                        add(to);
                    } else if (tc == Piece::None && to == epSq) {
                        add(to);
                    }
                }
                break;
            }
            case PieceType::Knight: {
                static const int kn[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
                for (auto &d : kn) {
                    addStep(moves, sq, f + d[0], r + d[1], white);
                }
                break;
            }
            case PieceType::Bishop: slide(moves, sq, white, true, false); break;
            case PieceType::Rook: slide(moves, sq, white, false, true); break;
            case PieceType::Queen: slide(moves, sq, white, true, true); break;
            case PieceType::King: {
                for (int df = -1; df <= 1; ++df) {
                    for (int dr = -1; dr <= 1; ++dr) {
                        if (df == 0 && dr == 0) continue;
                        addStep(moves, sq, f + df, r + dr, white);
                    }
                }
                int base = white ? 0 : 56;
                Piece rook = makePiece(PieceType::Rook, white);
                bool canK = white ? whiteCastleK : blackCastleK;
                bool canQ = white ? whiteCastleQ : blackCastleQ;
                if (sq == base + 4 && !isSquareAttacked(sq, !white)) {
                    if (canK && pieceAt(base + 7) == rook &&
                        pieceAt(base + 5) == Piece::None &&
                        pieceAt(base + 6) == Piece::None &&
                        !isSquareAttacked(base + 5, !white) &&
                        !isSquareAttacked(base + 6, !white)) {

                        moves.emplace_back(sq, base + 6);
                    }
                    if (canQ && pieceAt(base + 0) == rook &&
                        pieceAt(base + 1) == Piece::None &&
                        pieceAt(base + 2) == Piece::None &&
                        pieceAt(base + 3) == Piece::None &&
                        !isSquareAttacked(base + 3, !white) &&
                        !isSquareAttacked(base + 2, !white)) {

                        moves.emplace_back(sq, base + 2);
                    }
                }
                break;
            }
            case PieceType::None: break;
        }
    }
    return moves;
}

std::vector<Move> Board::legalMoves(bool white) const {
    std::vector<Move> legal;
    for (const Move &m : pseudoLegalMoves(white)) {
        Board copy = *this;
        copy.makeMove(m);
        if (!copy.inCheck(white)) legal.push_back(m);
    }
    return legal;
}

bool Board::isCheckmate(bool white) const {
    return inCheck(white) && legalMoves(white).empty();
}

bool Board::isStalemate(bool white) const {
    return !inCheck(white) && legalMoves(white).empty();
}

bool Board::insufficientMaterial() const {
    for (PieceType t : {PieceType::Pawn, PieceType::Rook, PieceType::Queen}) {
        if (bitboardFor(makePiece(t, true)) || bitboardFor(makePiece(t, false))) return false;
    }
    return popCount(bitboardFor(Piece::WhiteKnight) | bitboardFor(Piece::WhiteBishop) |
                    bitboardFor(Piece::BlackKnight) | bitboardFor(Piece::BlackBishop)) <= 1;
}

void Board::makeMove(const Move &m) {
    Piece piece = pieceAt(m.from);
    if (piece == Piece::None) return;
    bool white = isWhite(piece);
    PieceType type = pieceType(piece);
    Piece target = pieceAt(m.to);
    int fromFile = m.from & 7, toFile = m.to & 7;

    // en passant
    if (type == PieceType::Pawn && fromFile != toFile && target == Piece::None) {
        int capturedSq = white ? m.to - 8 : m.to + 8;
        clearSquare(capturedSq);
    }

    // King moves two files, bringing the rook across
    if (type == PieceType::King && std::abs(toFile - fromFile) == 2) {
        if (toFile > fromFile) {
            movePiece(m.from + 3, m.from + 1);
        } else {
            movePiece(m.from - 4, m.from - 1);
        }
    }
    movePiece(m.from, m.to);

    //promo
    if (type == PieceType::Pawn && ((m.to >> 3) == 7 || (m.to >> 3) == 0)) {
        PieceType promo = m.promo != PieceType::None ? m.promo : PieceType::Queen;
        clearSquare(m.to);
        putPiece(m.to, makePiece(promo, white));
    }

    // move a king/rook, or taking a rook on its home square
    if (m.from == 4 || m.to == 4) {
        whiteCastleK = whiteCastleQ = false;
    }
    if (m.from == 60 || m.to == 60) {
        blackCastleK = blackCastleQ = false;
    }
    if (m.from == 0 || m.to == 0) {
        whiteCastleQ = false;
    }
    if (m.from == 7 || m.to == 7) {
        whiteCastleK = false;
    }
    if (m.from == 56 || m.to == 56) {
        blackCastleQ = false;
    }
    if (m.from == 63 || m.to == 63) {
        blackCastleK = false;
    }

    //en passant target is only available for one move after the double step btw
    epSq = -1;
    if (type == PieceType::Pawn && std::abs(m.to - m.from) == 16) {
        epSq = (m.from + m.to) / 2;
    }
}

// Private methods

void Board::slide(std::vector<Move> &moves, int sq, bool white, bool diag, bool straight) const {
    static const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};

    for (int i = 0; i < 8; ++i) {
        if (i < 4 && !straight) continue;
        if (i >= 4 && !diag) continue;
        int nf = (sq & 7) + dirs[i][0], nr = (sq >> 3) + dirs[i][1];
        while (inBounds(nf, nr)) {
            int to = nr * 8 + nf;
            Piece tc = pieceAt(to);
            if (tc == Piece::None) {
                moves.emplace_back(sq, to);
            } else {
                if (isWhite(tc) != white) {
                    moves.emplace_back(sq, to);
                }
                break;
            }
            nf += dirs[i][0]; nr += dirs[i][1];
        }
    }
}

bool Board::inBounds(int f, int r) {
    return f >= 0 && f < 8 && r >= 0 && r < 8;
}

int Board::popCount(Bitboard bb) {
    int n = 0;
    while (bb) {
        bb &= bb - 1; ++n;
    }
    return n;
}

int Board::kingSquare(bool white) const {
    Bitboard k = bitboardFor(makePiece(PieceType::King, white));
    for (int sq = 0; sq < 64; ++sq) {
        if (getBit(k, sq)) return sq;
    }
    return -1;
}

void Board::addStep(std::vector<Move> &moves, int sq, int nf, int nr, bool white) const {
    if (!inBounds(nf, nr)) return;
    int to = nr * 8 + nf;
    Piece tc = pieceAt(to);
    if (tc == Piece::None || isWhite(tc) != white) {
        moves.emplace_back(sq, to);
    }
}

void Board::clearSquare(int square) {
    Piece piece = mailbox[square];
    if (piece == Piece::None) return;
    clearBit(bitboardFor(piece), square);
    clearBit(isWhite(piece) ? whiteOcc : blackOcc, square);
    clearBit(allOcc, square);
    mailbox[square] = Piece::None;
}

// Square must be empty
void Board::putPiece(int square, Piece piece) {
    setBit(bitboardFor(piece), square);
    setBit(isWhite(piece) ? whiteOcc : blackOcc, square);
    setBit(allOcc, square);
    mailbox[square] = piece;
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
