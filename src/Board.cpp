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
                    case 'N': {
                        static const int kn[8][2] = {{1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2}};
                        for (auto &d : kn) {
                            addStep(moves, sq, f + d[0], r + d[1], white);
                        }
                        break;
                    }
                    case 'B': slide(moves, sq, white, true, false); break;
                    case 'R': slide(moves, sq, white, false, true); break;
                    case 'Q': slide(moves, sq, white, true, true); break;
                    case 'K': {
                        for (int df = -1; df <= 1; ++df) {
                            for (int dr = -1; dr <= 1; ++ dr) {
                                if (df == 0 && dr == 0) continue;
                                addStep(moves, sq, f + df, r + dr, white);
                            }
                            int base = white ? 0 : 56; // Castle
                            char rook = white ? 'R' : 'r';
                            bool canK = white ? whiteCastleK : blackCastleK;
                            bool canQ = white ? whiteCastleQ : blackCastleQ;
                            if (sq == base + 4 && !isSquareAttacked(sq, !white)) {
                                if (canK && pieceCharAt(base + 7) == rook &&
                                    pieceCharAt(base + 5) == '.' &&
                                    pieceCharAt(base + 6) == '.' &&
                                    !isSquareAttacked(base + 5, !white) &&
                                    !isSquareAttacked(base + 6, !white)) {

                                    moves.emplace_back(sq, base + 6);
                                }
                                if (canQ && pieceCharAt(base + 0) == rook &&
                                    pieceCharAt(base + 1) == '.' &&
                                    pieceCharAt(base + 2) == '.' &&
                                    pieceCharAt(base + 3) == '.' &&
                                    !isSquareAttacked(base + 3, !white) &&
                                    !isSquareAttacked(base + 2, !white)) {

                                        moves.emplace_back(sq, base + 2);
                                }
                            }
                            break;
                        }
                    }
                }
            }
            return moves;
        }

        std::vector<Move> legalMoves(bool white) const {
            std::vector<Move> legal;
            for (const Move &m : pseudoLegalMoves(white)) {
                Board copy = *this;
                copy.makeMove(m);
                if (!copy.inCheck(white)) legal.push_back(m);
            }
            return legal;
        }

        bool isCheckmate(bool white) const {
            return inCheck(white) && legalMoves(white).empty();
        }

        bool isStalemate(bool white) const {
            return !inCheck(white) && legalMoves(white).empty();
        }

        bool insufficientMaterial() const {
            if (whitePawns || blackPawns || whiteRooks || blackRooks || whiteQueens || blackQueens) return false;
            return popCount(whiteKnights | whiteBishops | blackKnights | blackBishops ) <= 1;
        }
    
        static bool isWhitePiece(char c) {
            return c >= 'A' && c <= 'Z';
        }

        void makeMove(const Move &m) {
            char piece = pieceCharAt(m.from);
            if (piece == '.') return;
            bool white = isWhitePiece(piece);
            char type = static_cast<char>(std::toupper(piece));
            char target = pieceCharAt(m.to);
            int fromFile = m.from & 7, toFile = m.to & 7;

            // en passant
            if (type == 'P' && fromFile != toFile && target == '.') {
                int capturedSq = white ? m.to - 8 : m.to + 8;
                clearSquare(capturedSq);
            }

            // King moves two files, brining the rook across
            if (type == 'K' && std::abs(toFile - fromFile) == 2) {
                if (toFile > fromFile) {
                    movePiece(m.from + 3, m.from + 1);
                } else {
                    movePiece(m.from - 4, m.from - 1);
                }
            }
            movePiece(m.from, m.to);

            //promo
            if (type == 'P' && ((m.to >> 3) == 7 || (m.to >> 3) == 0)) {
                char promo = m.promo ? static_cast<char>(std::toupper(m.promo)) : 'Q';
                clearSquare(m.to);
                setBit(bitboardFor(white ? promo : static_cast<char>(std::tolower(promo))), m.to);
            }

            // move a king/rook, or taking a rook on its home square
            if (m.from == 4 || m.to == 4) {
                whiteCastleK = whiteCastleQ = false;
            }
            if (m.from == 60 || m.to == 60) { 
                blackCastleK = blackCastleQ = false; 
            }
            if (m.from == 0  || m.to == 0)  {
                whiteCastleQ = false;
            }
            if (m.from == 7  || m.to == 7) {
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
            if (type == 'P' && std::abs(m.to - m.from) == 16) {
                epSq = (m.from + m.to) / 2;
            }

        }



    private:

        void slide(std::vector<Move> &moves, int sq, bool white, bool diag, bool straight) const {
            static const int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
            
            for (int i = 0; i < 8; ++i) {
                if (i < 4 && !straight) continue;
                if (i >= 4 && !diag) continue;
                int nf = (sq & 7) + dirs[i][0], nr = (sq >> 3) + dirs[i][1];
                while (inBounds(nf, nr)) {
                    int to = nr * 8 + nf;
                    char tc = pieceCharAt(to);
                    if (tc == '.') {
                        moves.emplace_back(sq, to);
                    } else {
                        if (isWhitePiece(tc) != white) {
                            moves.emplace_back(sq, to);
                        }
                        break;
                    }
                    nf += dirs[i][0]; nr += dirs[i][1];
                }
            }
        }

        static bool inBounds(int f, int r) {
            return f >= 0 && f < 8 && r >= 0 && r < 8;
        }

        static int popCount(Bitboard bb) {
            int n = 0;
            while(bb) {
                bb &= bb - 1; ++n;
            }
            return n;
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
        
        void addStep(std::vector<Move> &moves, int sq, int nf, int nr, bool white) const {
            if (!inBounds(nf, nr)) return;
            int to = nr * 8 + nf;
            char tc = pieceCharAt(to);
            if (tc == '.' || isWhitePiece(tc) != white) {
                moves.emplace_back(sq, to);
            }
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
