#include <iostream>
#include <cstdint>
#include <array>
#define BOARD_CPP

using Bitboard = uint64_t;
struct Move {
    int from;
    int to;
    char promo;
    Move(int f = 0, int t = 0, char p = 0) : from(f), to(t), promo(p) {}
};

class Board {
    Bitboard whitePawns = 0; // Creating bitboards for each piece type
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

    bool whiteCastleK = false; // Castling rights, and which side. (TODO: IMPLEMENT LOGIC)
    bool whiteCastleQ = false;
    bool blackCastleK = false;
    bool blackCastleQ = false;

    int epSq = -1; // The square a pawn skipped over on the last move. For en passants (TODO: IMPLEMENT LOGIC)

    public:
        static bool getBit(Bitboard bb, int square) {
            return (bb >> square) &1ULL;
        }

        static void setBit(Bitboard &bb, int square) {
            bb |= 1ULL << square;
        }

        static void clearBit(Bitboard &bb, int square) {
            bb &= ~(1ULL << square);
        }

        void printBoard() const {
            std::cout << "\n    a b c d e f g h\n";
            std::cout << "    ---------------\n";
            for (int rank = 7; rank >= 0; --rank) {
                std::cout << (rank + 1) << " |"<< " ";
                for (int file = 0; file < 8; ++file) {
                    int square = rank * 8 + file;
                    char piece = pieceCharAt(square);
                    std::cout << piece << ' ';
                }
                std::cout << '\n';
            }
        }

        void initPieces() {
            whitePawns = 0x000000000000FF00ULL; // Creating starting position of the pieces
            whiteKnights = 0x0000000000000042ULL;
            whiteRooks = 0x0000000000000081ULL;
            whiteBishops = 0x0000000000000024ULL;
            whiteQueens = 0x0000000000000008ULL;
            whiteKing = 0x0000000000000010ULL;

            blackPawns = 0x00FF000000000000ULL;
            blackKnights = 0x4200000000000000ULL;
            blackRooks = 0x8100000000000000ULL;
            blackBishops = 0x2400000000000000ULL;
            blackQueens = 0x0800000000000000ULL;
            blackKing = 0x1000000000000000ULL;

            whiteCastleK = whiteCastleQ = blackCastleK = blackCastleQ = true; // Setting rights to true
            epSq = -1;
        }

        void movePiece(int frSq, int toSq) {
            char piece = pieceCharAt(frSq);
            if (piece == '.') {
                std::cerr << "No piece on from square " << frSq << std::endl;
                return;
            }

            clearSquare(toSq);
            Bitboard &bb = bitboardFor(piece);
            moveOnBitBoard(bb, frSq, toSq); // Removed future repetative code to a funciton
        }

        char pieceCharAt(int square) const {
            if (getBit(whitePawns, square)) return 'P'; // Printing pieces from bitboard
            if (getBit(whiteKnights, square)) return 'N';
            if (getBit(whiteRooks, square)) return 'R';
            if (getBit(whiteBishops, square)) return 'B';
            if (getBit(whiteQueens, square)) return 'Q';
            if (getBit(whiteKing, square)) return 'K';
            if (getBit(blackPawns, square)) return 'p';
            if (getBit(blackKnights, square)) return 'n';
            if (getBit(blackRooks, square)) return 'r';
            if (getBit(blackBishops, square)) return 'b';
            if (getBit(blackQueens, square)) return 'q';
            if (getBit(blackKing, square)) return 'k';

            return '.';
        }

        // RULES

        bool isSquareAttacked(int sq, bool byWhite) const {
            const int f = sq & 7, r = sq >> 3;
            const char P = byWhite ? 'P' : 'p';
            const char N = byWhite ? 'N' : 'n';
            const char B = byWhite ? 'B' : 'b';
            const char R = byWhite ? 'R' : 'r';
            const char Q = byWhite ? 'Q' : 'q';
            const char K = byWhite ? 'K' : 'k';

            //Pawns

            int pr = byWhite ? r-1 : r + 1;
            for (int df = -1; df <= 1; df += 2) {
                int pf = f + df;
                if (inBounds(pf, pr) && pieceCharAt(pr * 8 + pf) == P) return true;
            }

            // Knights
            static const int kn[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
            for (auto &d : kn) {
                int nf = f + d[0], nr = r + d[1];
                if (inBounds(nf,nr) && pieceCharAt(nr * 8 + nf) == N) return true;
            }

            //King

            for (int df = -1; df <= 1; ++df) {
                for (int dr = -1; dr<= 1; ++dr) {
                    if (df == 0 && dr == 0) continue;
                    int nf = f + df, nr = r + dr;
                    if (inBounds(nf, nr) && pieceCharAt(nr * 8 + nf) == K) return true; 
                }
            }

            // rook/queen straights & bishop/queen diagonals
            static const int straight[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
            static const int diagonal[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
            for (auto &d : straight) {
                int nf = f + d[0], nr = r + d[1];
                while (inBounds(nf,nr)) {
                    char c = pieceCharAt(nr * 8 + nf);
                    if (c != '.') {
                        if (c == R || c == Q) return true; break;
                    }
                    nf += d[0];
                    nr += d[1];
                }
            }
            for (auto &d : diagonal) {
                int nf = f + d[0], nr = r + d[1];
                while (inBounds(nf, nr)) {
                    char c = pieceCharAt(nr * 8 + nf);
                    if (c != '.') {
                        if (c == B || c == Q) return true; break;
                    }
                    nf += d[0];
                    nr += d[1];
                }
            }
            return false;
        }

        bool inCheck(bool white) const {
            int k = kingSquare(white);
            if (k < 0) return false;
            return isSquareAttacked(k, !white);
        }

        //legal moves
        std::vector<Move> pseudoLegalMoves(bool white) const {
            std::vector<Move> moves;
            for (int sq = 0; sq < 64; ++sq) {
                char c = pieceCharAt(sq);
                if (c == '.' || isWhitePiece(c) != white) continue;
                const int f = sq & 7, r = sq >> 3;

                switch (std::toupper(c)) {
                    case 'P': {
                        int dir = white ? 1 : -1;
                        int startRank = white ? 1 : 6;
                        int lastRank = white ? 7 : 0;
                        auto add = [&](int to) {
                            if ((to >> 3) == lastRank) {
                                for (char p : {'Q', 'R', 'B', 'N'}) { // promo
                                    moves.emplace_back(sq, to, p);
                                }
                            } else {
                                moves.emplace_back(sq, to);
                            }
                        };
                        int nr = r + dir;
                        if (nr < 0 || nr > 7) break;
                        int one = nr * 8 + f;
                        if (pieceCharAt(one) == '.') {
                            add(one);
                            int two = one + dir * 8;
                            if (r == startRank&& pieceCharAt(two) == '.') {
                                moves.emplace_back(sq, two);
                            }
                        }
                        for (int df = -1; df <= 1; df += 2) {
                            int nf = f + df;
                            if (nf < 0 || nf > 7) continue;
                            int to = nr * 8 + nf;
                            char tc = pieceCharAt(to);
                            if (tc != '.' && isWhitePiece(tc) != white) {
                                add(to);
                            } else if (tc == '.' && to == epSq) { // en passant
                                add(to);
                            }
                        }
                        break;
                    }
                }
            }
        }
    
        static bool isWhitePiece(char c) {
            return c >= 'A' && c <= 'Z';
        }
    private:

        static bool inBounds(int f, int r) {
            return f >= 0 && f < 8 && r >= 0 && r < 8;
        }

        int kingSquare(bool white) const {
            Bitboard k = white ? whiteKing : blackKing;
            for (int sq = 0; sq < 64; ++ sq) {
                if (getBit(k, sq)) return sq;
            }
            return -1;
        }

        static void moveOnBitBoard(Bitboard &bb, int frSq, int toSq) {
            clearBit(bb, frSq);
            setBit(bb, toSq);
        }

        void clearSquare(int square){
            clearBit(whitePawns, square);
            clearBit(whiteKnights, square);
            clearBit(whiteRooks, square);
            clearBit(whiteBishops, square);
            clearBit(whiteQueens, square);
            clearBit(whiteKing, square);
            clearBit(blackPawns, square);
            clearBit(blackKnights, square);
            clearBit(blackRooks, square);
            clearBit(blackBishops, square);
            clearBit(blackQueens, square);
            clearBit(blackKing, square);
        }

        Bitboard &bitboardFor(char piece) {
            switch (piece) {
                case 'P': return whitePawns;
                case 'N': return whiteKnights;
                case 'R': return whiteRooks;
                case 'B': return whiteBishops;
                case 'Q': return whiteQueens;
                case 'K': return whiteKing;
                case 'p': return blackPawns;
                case 'n': return blackKnights;
                case 'r': return blackRooks;
                case 'b': return blackBishops;
                case 'q': return blackQueens;
                case 'k': return blackKing;
                default: return blackKing;
            }
        }

};
