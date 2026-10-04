#include <iostream>
#include <cstdint>
#include <array>
#define BOARD_CPP

using Bitboard = uint64_t;

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
        }

        void movePiece(int frSq, int toSq) {
            char piece = pieceCharAt(frSq);
            if (piece == '.') {
                std::cerr << "No piece on from square " << frSq << std::endl;
                return;
            }

            clearSquare(toSq);
            switch (piece) {
                case 'P': moveOnBitBoard(whitePawns, frSq, toSq); break;
                case 'N': moveOnBitBoard(whiteKnights, frSq, toSq); break;
                case 'R': moveOnBitBoard(whiteRooks, frSq, toSq); break;
                case 'B': moveOnBitBoard(whiteBishops, frSq, toSq); break;
                case 'Q': moveOnBitBoard(whiteQueens, frSq, toSq); break;
                case 'K': moveOnBitBoard(whiteKing, frSq, toSq); break;
                case 'p': moveOnBitBoard(blackPawns, frSq, toSq); break;
                case 'n': moveOnBitBoard(blackKnights, frSq, toSq); break;
                case 'r': moveOnBitBoard(blackRooks, frSq, toSq); break;
                case 'b': moveOnBitBoard(blackBishops, frSq, toSq); break;
                case 'q': moveOnBitBoard(blackQueens, frSq, toSq); break;
                case 'k': moveOnBitBoard(blackKing, frSq, toSq); break;
            }
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
    
    private:
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

};
