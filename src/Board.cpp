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
            whitePawns = 0x000000000000FF00ULL;
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
    
    private:
        char pieceCharAt(int square) const {
            if (getBit(whitePawns, square)) return 'P';
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

};
